#ifndef GODOTPP_NET_MANAGER_H
#define GODOTPP_NET_MANAGER_H

#include <linking_context.h>

#include "snl.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "serialization/serializer.h"

namespace godot {
    class NetworkManager : public Node
    {
        GDCLASS(NetworkManager, Node)

    protected:
        String server_address = "127.0.0.1:5000";
        GameSocket *socket;

        LinkingContext linking_context;

        uint8_t read_buffer[1024];
        char sender_address[128];

        uint32_t ping_id_counter = 0;
        float ping_timer = 0.0f;

    public:
        NetworkManager();
        ~NetworkManager();

        void _ready() override;

        void _process(double delta) override;

        int try_connect(const String &address);

        void send_packet(const uint8_t* data, size_t size);

        void disconnect();

    protected:
        void process_snapshot(StreamReader& reader);

        static void _bind_methods();
    };
}

#endif //GODOTPP_NET_MANAGER_H