#include "net_manager.h"

#include <random>
#include <utils.h>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

#include "net_serializer.h"

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
        net_socket_send(socket, server_address.utf8().get_data(), (uint8_t*)&packet, sizeof(HelloPacket));

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

            PacketType packet_type = (PacketType)read_buffer[0];

            if (packet_type == PacketType::WORLD)
            {
                WorldPacket* packet = reinterpret_cast<WorldPacket*>(read_buffer);
                if (bytes_read >= sizeof(WorldPacket))
                {
                    Node* netNode = linking_context.get_node(packet->netID);
                    if (netNode == nullptr)
                    {
                        Node* spawned_node = linking_context.spawn_network_object(packet->netID, packet->typeID);
                        if (spawned_node)
                        {
                            add_child(spawned_node);
                            Node2D* spawned_node_2d = dynamic_cast<Node2D*>(spawned_node);
                            if (spawned_node_2d != nullptr) spawned_node_2d->set_position(Vector2(packet->x, packet->y));
                            UtilityFunctions::print("[CLIENT] Spawned ID: ", packet->netID, " at: ", packet->x, ", ", packet->y);
                        }
                    }
                    else
                    {
                        Node2D* node = dynamic_cast<Node2D*>(netNode);
                        node->set_position(Vector2(packet->x, packet->y));
                    }
                }
            }
            else if (packet_type == PacketType::PONG)
            {
                PingResponse* pong_packet = reinterpret_cast<PingResponse*>(read_buffer);

                uint64_t t2 = now_ms();
                uint64_t rtt = t2 - pong_packet->t0;

                UtilityFunctions::print("[PING #", pong_packet->id, "] RTT = ", rtt, " ms");
            }
            else if (packet_type == PacketType::WORLD_SNAPSHOT)
            {
                WorldSnapshotPacket* snapshot_packet = reinterpret_cast<WorldSnapshotPacket*>(read_buffer);
                NetReader reader(snapshot_packet->data);
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

            send_packet((uint8_t*)&ping_packet, sizeof(PingRequest));
        }
    }
}

void NetworkManager::process_snapshot(NetReader& reader)
{
    uint32_t entity_count = 0;
    reader >> entity_count;

    for (uint32_t i = 0; i < entity_count; ++i) {
        NetID netID = 0;
        TypeID typeID = 0;
        Vec2 position {0, 0};

        reader >> netID;
        reader >> typeID;
        reader >> position;

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