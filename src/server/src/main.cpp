#include <iostream>
#include <snl.h>

int main() {
    std::cout << "Init server" << std::endl;

    GameSocket* socket = net_socket_create("0.0.0.0:5000");
    std::cout << "Listening to port 5000" << std::endl;

    uint8_t read_buffer[1024];
    char sender_address[128];

    while (true) {
        int32_t bytes_read = net_socket_poll(socket, read_buffer, 1024, sender_address, 128);
        if (bytes_read > 0) { // There is data
            std::cout << "Packet received: " << bytes_read << std::endl;
        }
    }

    return 0;
}