#include <iostream>
#include <snl.h>
#include <vector>
#include <cstring>
#include <algorithm>

#include "../../shared/include/net_protocol.h"

struct Client
{
    char address[128];
    uint32_t id;
};

int main() {
    std::cout << "Init server" << std::endl;

    GameSocket* socket = net_socket_create("0.0.0.0:5000");
    std::cout << "Listening to port 5000" << std::endl;

    std::vector<Client> clients;
    uint32_t next_id = 100;

    uint8_t read_buffer[1024];
    char sender_address[128];

    while (true) {
        int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
        if (bytes_read > 0) { // There is data
            auto it = std::find_if(clients.begin(), clients.end(), [&](const Client& c)
            {
               if (std::strncmp(sender_address, c.address, 128) == 0)
               {
                   return true;
               } else return false;
            });

            bool is_new_client = (it == clients.end());

            if (is_new_client)
            {
                std::cout << "[SERVER] New connection from " << sender_address << std::endl;

                Client new_client;
                memcpy(new_client.address, sender_address, sizeof(new_client.address));
                new_client.id = next_id;
                clients.push_back(new_client);
                ++next_id;

                std::cout << "[SERVER] Spawn Node ID " << new_client.id << std::endl;

                SpawnPacket packet;
                packet.type = PacketType::SPAWN;
                packet.netID = new_client.id;
                packet.typeID = 1;

                for (const auto& client : clients)
                {
                    net_socket_send(socket, client.address, (uint8_t*)&packet, sizeof(SpawnPacket));
                }

                // TODO: Send previously connected clients to new client
            }
            else
            {
                // TODO: Handle client packet
                std::cout << "[SERVER] Old connection from " << sender_address << std::endl;
            }
        }
    }

    return 0;
}