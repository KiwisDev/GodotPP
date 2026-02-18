#include "net_manager.h"

#include <godot_cpp/classes/resource_loader.hpp>

godot::NetworkManager::NetworkManager() {}

godot::NetworkManager::~NetworkManager() {}

void godot::NetworkManager::_ready()
{
    Node::_ready();

    socket = net_socket_create("127.0.0.1:0");

    if (socket) {
        // TODO: Send Spawn packet
        uint8_t data[1] = {0x48};
        net_socket_send(socket, "127.0.0.1:5000", data, sizeof(data));
    }

    linking_context = LinkingContext();
    linking_context.register_type(1, []() -> Node*
    {
        Ref<PackedScene> player_scene = ResourceLoader::get_singleton()->load("res://player.tscn");
        return player_scene->instantiate();
    });
}

void godot::NetworkManager::_process(double delta)
{
    Node::_process(delta);

    int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
    if (bytes_read > 0) { // There is data
        PacketType packet_type = (PacketType)read_buffer[0];

        if (packet_type == PacketType::SPAWN)
        {
            SpawnPacket* packet = reinterpret_cast<SpawnPacket*>(read_buffer);
            if (bytes_read >= sizeof(SpawnPacket))
            {
                Node* spawned_node = linking_context.spawn_network_object(packet->netID, packet->typeID);
                if (spawned_node)
                {
                    add_child(spawned_node);
                    UtilityFunctions::print("[CLIENT] Spawned ID: ", packet->netID);
                }
            }
        }
    }
}

void godot::NetworkManager::_bind_methods() {}