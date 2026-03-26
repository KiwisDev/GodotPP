#include "net_manager.h"

#include <random>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

godot::NetworkManager::NetworkManager() {}

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
        set_process(true);

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

    if (socket)
    {
        int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
        if (bytes_read > 0) { // There is data
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
