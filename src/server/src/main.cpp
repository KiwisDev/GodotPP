#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <thread>

#include "snl.h"
#include "net_protocol.h"
#include "net_serializer.h"
#include "scene/scene.h"

int main() {
    std::cout << "Init server" << std::endl;

    GameSocket* socket = net_socket_create("0.0.0.0:5000");
    std::cout << "Listening to port 5000" << std::endl;

    Scene world;
    uint32_t next_userID = 100;
    uint32_t next_netID = 1;

    uint8_t read_buffer[1024];
    char sender_address[128];

    using clock = std::chrono::steady_clock;
    const std::chrono::duration<double> target_duration(1.0 / 60.0);

    while (true) {
        auto frame_start = clock::now();

        int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
        if (bytes_read > 0) { // There is data
            PacketType packet_type = (PacketType)read_buffer[0];

            // Hello packet
            if (packet_type == PacketType::HELLO) {
                HelloPacket* hello_packet = reinterpret_cast<HelloPacket*>(read_buffer);

                auto clients_view = world.registry.view<ClientComponent>();
                bool is_new_client = true;

                for (auto entity : clients_view)
                {
                    const auto& client = clients_view.get<ClientComponent>(entity);
                    if (std::strncmp(sender_address, client.address, 128) == 0)
                    {
                        is_new_client = false;
                        break;
                    }
                }

                if (is_new_client)
                {
                    std::cout << "[SERVER] New connection from " << sender_address << std::endl;

                    std::cout << "[SERVER] Spawn Node ID " << next_netID << " at " << hello_packet->x << " " << hello_packet->y << std::endl;

                    float pos_x = hello_packet->x; float pos_y = hello_packet->y;
                    entt::entity new_player_entity = world.registry.create();
                    world.registry.emplace<NetworkComponent>(new_player_entity, next_netID);
                    world.registry.emplace<TransformComponent>(new_player_entity, Vec2{pos_x, pos_y});
                    world.registry.emplace<VelocityComponent>(new_player_entity);
                    world.registry.emplace<ClientComponent>(new_player_entity, sender_address, next_netID);

                    // Send a spawn packet to all connected clients
                    SpawnPacket packet;
                    packet.type = PacketType::SPAWN;
                    packet.netID = next_netID;
                    packet.typeID = 1;
                    packet.x = hello_packet->x;
                    packet.y = hello_packet->y;

                    for (auto& entity : clients_view)
                    {
                        const auto& client = clients_view.get<ClientComponent>(entity);
                        net_socket_send(socket, client.address, (uint8_t*)&packet, sizeof(SpawnPacket));
                    }

                    ++next_userID;
                    ++next_netID;

                    auto view = world.registry.view<NetworkComponent, TransformComponent>();
                    for (auto [entity, network, transform] : view.each())
                    {
                        if (network.id != packet.netID)
                        {
                            SpawnPacket new_packet;
                            new_packet.type = PacketType::SPAWN;
                            new_packet.netID = network.id;
                            new_packet.typeID = 1;
                            new_packet.x = transform.position.x;
                            new_packet.y = transform.position.y;

                            net_socket_send(socket, sender_address, (uint8_t*)&new_packet, sizeof(SpawnPacket));
                        }
                    }
                }
                else
                {
                    // TODO: Handle client packet
                    std::cout << "[SERVER] Old connection from " << sender_address << std::endl;
                }
            }
        }
        auto frame_end = clock::now();
        auto elapsed = frame_end - frame_start;

        if (elapsed < target_duration) {
            std::this_thread::sleep_for(target_duration - elapsed);
        }
    }

    return 0;
}