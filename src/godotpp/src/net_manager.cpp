#include "net_manager.h"

#include <random>
#include <utils.h>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "serialization/serializer.h"

godot::NetworkManager::NetworkManager() {
    set_process(false);
}

godot::NetworkManager::~NetworkManager() {}

void godot::NetworkManager::_ready()
{
    Node::_ready();
}

int NetworkManager::try_connect(const String &address)
{
    UtilityFunctions::print("[CLIENT] Called try_connect");
    server_address = address;
    socket = net_socket_create("127.0.0.1:0");

    linking_context = LinkingContext();
    linking_context.register_type(1, []() -> Node*
    {
        Ref<PackedScene> player_scene = ResourceLoader::get_singleton()->load("res://player.tscn");
        return player_scene->instantiate();
    });

    if (socket) {
        // Send hello packet with x y
        HelloPacket packet;
        packet.type = PacketType::HELLO;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(-512, 512);
        packet.x = distrib(gen);
        packet.y = distrib(gen) / 2;

        UtilityFunctions::print("[CLIENT] Send hello at: ", packet.x, ", ", packet.y);

        StreamWriter w;
        packet.serialize(w);
        std::vector<uint8_t> packet_data = w.finish();

        net_socket_send(socket, server_address.utf8().get_data(), packet_data.data(), packet_data.size());

        set_process(true);
    }
    else
    {
        UtilityFunctions::print("[CLIENT] Socket could not be created");
    }

    return 0;
}

void NetworkManager::send_packet(const uint8_t* data, size_t size)
{
    if (socket)
    {
        net_socket_send(socket, server_address.utf8().get_data(), data, size);
    }
    else
    {
        UtilityFunctions::print("[CLIENT] Tried to send packet but socket was invalid");
    }
}

void godot::NetworkManager::_process(double delta)
{
    Node::_process(delta);

    process_socket(delta);
    update_world(delta);
}

void NetworkManager::process_socket(double delta) {
    ping_timer += delta;

    if (socket)
    {
        while (true) {
            int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
            if (bytes_read <= 0)
            {
                break;
            }

            StreamReader r(read_buffer);
            auto const k_packet_type = r.read<uint8_t>();
            if (!k_packet_type)
            {
                UtilityFunctions::print("[CLIENT] Failed to read PacketType");
                continue;
            }
            auto packet_type = static_cast<PacketType>(*k_packet_type);

            if (packet_type == PacketType::PONG)
            {
                r = StreamReader(read_buffer);
                auto k_pong_packet = PingResponse::deserialize(r);
                if (!k_pong_packet)
                {
                    UtilityFunctions::print("[SERVER] Failed to read PingResponse");
                    continue;
                }
                PingResponse pong_packet = *k_pong_packet;

                uint64_t t2 = now_ms();
                uint64_t rtt = t2 - pong_packet.t0;

                UtilityFunctions::print("[PING #", pong_packet.id, "] RTT = ", rtt, " ms");
            }
            else if (packet_type == PacketType::WORLD_SNAPSHOT)
            {
                r = StreamReader(read_buffer);
                auto k_snapshot_packet = WorldSnapshotPacket::deserialize(r);
                if (!k_snapshot_packet)
                {
                    UtilityFunctions::print("[CLIENT] Failed to read WorldSnapshotPacket");
                    continue;
                }
                WorldSnapshotPacket snapshot_packet = *k_snapshot_packet;

                if (snapshots_history.size() == 0 || (snapshots_history[snapshots_history.size()-1].frame_number < snapshot_packet.frame_number))
                {
                    snapshots_history.push_back(WorldSnapshot{snapshot_packet.frame_number});
                    if (snapshots_history.size() > snapshots_buffer_size)
                    {
                        snapshots_history.pop_front();
                    }
                    StreamReader reader(snapshot_packet.data);
                    process_snapshot(reader, snapshots_history[snapshots_history.size()-1]);
                }
            }
        }

        if (ping_timer >= 1.0f)
        {
            ping_timer = 0.0f;

            PingRequest ping_packet;
            ping_packet.type = PacketType::PING;
            ping_packet.id = ping_id_counter++;
            ping_packet.t0 = now_ms();

            StreamWriter w;
            ping_packet.serialize(w);
            std::vector<uint8_t> ping_data = w.finish();

            send_packet(ping_data.data(), ping_data.size());
        }
    }
}

void NetworkManager::update_world(double delta) {
    if (snapshots_history.size() >= snapshots_buffer_size) {
        if (interpolation_frame == 0)
        {
            auto it = std::min_element(snapshots_history.begin(), snapshots_history.end(),
            [](const WorldSnapshot& a, const WorldSnapshot& b) {
                    return a.frame_number < b.frame_number;
                }
            );

            interpolation_frame = it->frame_number;
        }
        else
        {
            interpolation_frame += (delta * 60.0);

            double newest_frame = snapshots_history.back().frame_number;

            if (interpolation_frame > newest_frame)
            {
                interpolation_frame = newest_frame;
            }
        }

        WorldSnapshot* frameA = nullptr;
        WorldSnapshot* frameB = nullptr;

        for (size_t i = 0; i < snapshots_history.size() - 1; ++i) {
            if (snapshots_history[i].frame_number <= interpolation_frame && snapshots_history[i + 1].frame_number >= interpolation_frame)
            {
                frameA = &snapshots_history[i];
                frameB = &snapshots_history[i + 1];
                break;
            }
        }

        if (frameA && frameB) {
            float difference_frames = static_cast<float>(frameB->frame_number - frameA->frame_number);
            float alpha = (interpolation_frame - frameA->frame_number) / difference_frames;

            //UtilityFunctions::print("[CLIENT] Interp frame: ", interpolation_frame, " Frame 1: ", frameA->frame_number, " Frame 2: ", frameB->frame_number, " Alpha: ", alpha);

            std::unordered_map<NetID, const EntitySnapshot*> entities_B;
            entities_B.reserve(frameB->entities.size());

            for (const auto& entB : frameB->entities)
            {
                entities_B[entB.net_id] = &entB;
            }

            for (const auto& entA : frameA->entities)
            {
                auto it = entities_B.find(entA.net_id);

                if (it != entities_B.end())
                {
                    Node* netNode = linking_context.get_node(entA.net_id);
                    if (netNode != nullptr)
                    {
                        const EntitySnapshot* entB = it->second;
                        glm::vec2 interp_pos = glm::mix(entA.position, entB->position, alpha);

                        Node2D* node = dynamic_cast<Node2D*>(netNode);
                        node->set_position(Vector2(interp_pos.x, interp_pos.y));

                        entities_B.erase(it);
                    }
                }
                else
                {
                    // Entity has been deleted
                }
            }

            for (const auto& pair : entities_B)
            {
                const EntitySnapshot* new_entB = pair.second;
                Node* spawned_node = linking_context.spawn_network_object(new_entB->net_id, new_entB->type_id);
                if (spawned_node)
                {
                    add_child(spawned_node);
                    Node2D* spawned_node_2d = dynamic_cast<Node2D*>(spawned_node);
                    if (spawned_node_2d != nullptr) spawned_node_2d->set_position(Vector2(new_entB->position.x, new_entB->position.y));
                    UtilityFunctions::print("[CLIENT] Spawned ID: ", new_entB->net_id, " at: ", new_entB->position.x, ", ", new_entB->position.y);
                }
            }
        }
        else {UtilityFunctions::print("UH OH");}
    }
}

void NetworkManager::process_snapshot(StreamReader& reader, WorldSnapshot& snapshot)
{
    auto entity_count_opt = reader.read<uint32_t>();
    if (!entity_count_opt)
    {
        UtilityFunctions::print("[CLIENT] No entity count in received snapshot, skipping this snapshot");
        return;
    }
    uint32_t entity_count = entity_count_opt.value();

    for (uint32_t i = 0; i < entity_count; ++i) {
        auto const k_netID = reader.read<uint32_t>();
        auto const k_typeID = reader.read<uint32_t>();
        auto const k_position = reader.read_vec2_quantized(-5000.0f, 5000.0f, 2);

        if (!k_netID || !k_typeID || !k_position) {
            UtilityFunctions::print("[CLIENT] Failed to read a EntitySnapshot");
            continue;
        }

        EntitySnapshot entity_snapshot{*k_netID, *k_typeID, std::move(*k_position)};
        snapshot.entities.push_back(entity_snapshot);
    }
}

void NetworkManager::disconnect()
{
    UtilityFunctions::print("[CLIENT] Disconnect placeholder");
}

void godot::NetworkManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("try_connect", "address"), &NetworkManager::try_connect);
    ClassDB::bind_method(D_METHOD("disconnect"), &NetworkManager::disconnect);
}