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
                    UtilityFunctions::print("[SERVER] Failed to read WorldSnapshotPacket");
                    continue;
                }
                WorldSnapshotPacket snapshot_packet = *k_snapshot_packet;
                StreamReader reader(snapshot_packet.data);
                process_snapshot(reader);
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

void NetworkManager::process_snapshot(StreamReader& reader)
{
    auto entity_count_opt = reader.read<uint32_t>();
    if (!entity_count_opt)
    {
        UtilityFunctions::print("[CLIENT] No entity count in received snapshot, skipping this snapshot");
        return;
    }
    uint32_t entity_count = entity_count_opt.value();

    for (uint32_t i = 0; i < entity_count; ++i) {

        NetID netID = reader.read<uint32_t>().value();
        TypeID typeID = reader.read<uint32_t>().value();
        glm::vec2 position = reader.read_vec2_quantized(-5000.0f, 5000.0f, 2).value();

        Node* netNode = linking_context.get_node(netID);
        if (netNode == nullptr)
        {
            Node* spawned_node = linking_context.spawn_network_object(netID, typeID);
            if (spawned_node)
            {
                add_child(spawned_node);
                Node2D* spawned_node_2d = dynamic_cast<Node2D*>(spawned_node);
                if (spawned_node_2d != nullptr) spawned_node_2d->set_position(Vector2(position.x, position.y));
                UtilityFunctions::print("[CLIENT] Spawned ID: ", netID, " at: ", position.x, ", ", position.y);
            }
        }
        else
        {
            Node2D* node = dynamic_cast<Node2D*>(netNode);
            node->set_position(Vector2(position.x, position.y));
        }
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