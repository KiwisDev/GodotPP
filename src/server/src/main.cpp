#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

#include "snl.h"
#include "net_protocol.h"
#include "net_serializer.h"
#include "scene/scene.h"
#include "utils.h"

void process_client_input(TransformComponent& transform, const InputData& input);

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

    while (true)
    {
        auto frame_start = clock::now();

        while (true) {
            int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
            if (bytes_read <= 0)
            {
                break;
            }

            PacketType packet_type = (PacketType)read_buffer[0];

            // Hello packet
            if (packet_type == PacketType::HELLO)
            {
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

                    ++next_userID;
                    ++next_netID;
                }
            }
            else if (packet_type == PacketType::INPUT)
            {
                InputPacket* input_packet = reinterpret_cast<InputPacket*>(read_buffer);

                auto view = world.registry.view<ClientComponent, TransformComponent>();

                // Get the entity associated with the IP
                for (auto entity : view)
                {
                    auto& client = view.get<ClientComponent>(entity);
                    if (std::strncmp(sender_address, client.address, 128) == 0)
                    {
                        for (int i=0; i < 20; ++i)
                        {
                            const InputData& input = input_packet->input_history[i];

                            if (!client.is_first_input_packet && input.sequence <= client.last_processed_sequence) continue;

                            auto& transform = view.get<TransformComponent>(entity);
                            process_client_input(transform, input);

                            client.last_processed_sequence = input.sequence;
                            client.is_first_input_packet = false;
                        }

                        break;
                    }
                }
            }
            else if (packet_type == PacketType::PING)
            {
                PingRequest* ping_packet = reinterpret_cast<PingRequest*>(read_buffer);

                uint64_t t1 = now_ms();

                PingResponse pong_packet;
                pong_packet.type = PacketType::PONG;
                pong_packet.id = ping_packet->id;
                pong_packet.t0 = ping_packet->t0;
                pong_packet.t1 = t1;

                net_socket_send(socket, sender_address, (uint8_t*)&pong_packet, sizeof(PingResponse));
            }
        }

        // Send world state
        std::vector<std::string> address_list;
        auto clients_view = world.registry.view<ClientComponent>();
        for (auto entity : clients_view)
        {
            auto& client = clients_view.get<ClientComponent>(entity);
            address_list.emplace_back(client.address);
        }

        auto world_view = world.registry.view<TransformComponent, NetworkComponent>();
        for (auto client_address : address_list)
        {
            char char_address[128];
            std::strncpy(char_address, client_address.c_str(), 128);
            for (auto [entity, transform, network] : world_view.each())
            {
                WorldPacket world_packet;
                world_packet.type = PacketType::WORLD;
                world_packet.netID = network.id;
                world_packet.typeID = 1;
                world_packet.x = static_cast<int16_t>(transform.position.x);
                world_packet.y = static_cast<int16_t>(transform.position.y);

                net_socket_send(socket, char_address, (uint8_t*)&world_packet, sizeof(WorldPacket));
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

void process_client_input(TransformComponent& transform, const InputData& input)
{
    std::cout << "[SERVER] Received client input: " << input.sequence << std::endl;
    bool up = (input.keys & FLAG_UP) != 0;
    bool down = (input.keys & FLAG_DOWN) != 0;
    bool left = (input.keys & FLAG_LEFT) != 0;
    bool right = (input.keys & FLAG_RIGHT) != 0;
    bool action = (input.keys & FLAG_ACTION) != 0;

    if (up) transform.position.y -= 50 * (1.0f/60.0f);
    if (down) transform.position.y += 50 * (1.0f/60.0f);
    if (left) transform.position.x -= 50 * (1.0f/60.0f);
    if (right) transform.position.x += 50 * (1.0f/60.0f);
}