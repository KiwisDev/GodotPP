#ifndef GODOTPP_NET_MANAGER_H
#define GODOTPP_NET_MANAGER_H

#include <deque>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

#include "serialization/serializer.h"
#include "linking_context.h"
#include "snl.h"

struct EntitySnapshot {
    NetID net_id;
    TypeID type_id;
    glm::vec2 position;
};

struct WorldSnapshot {
    uint64_t frame_number;
    std::vector<EntitySnapshot> entities;
};

namespace godot {
    class PlayerController;

    class NetworkManager : public Node
    {
        GDCLASS(NetworkManager, Node)

    protected:
        PlayerController* player_controller_uh = nullptr;

        String server_address = "127.0.0.1:5000";
        GameSocket *socket;

        LinkingContext linking_context;

        uint8_t read_buffer[1024];
        char sender_address[128];

        uint32_t ping_id_counter = 0;
        float ping_timer = 0.0f;
        uint64_t rtt = 0;

        std::deque<WorldSnapshot> snapshots_history;
        double interpolation_frame = 0;
        uint8_t snapshots_buffer_size = 10;

        uint64_t server_frame = 0;
        uint32_t last_server_process_input = 0;

        NetID myNetID = 0;

    public:
        NetworkManager();
        ~NetworkManager();

        void _ready() override;

        void _process(double delta) override;

        int try_connect(const String &address);

        void send_packet(const uint8_t* data, size_t size);

        void disconnect();

        uint64_t get_local_player_net_id() const { return myNetID; }

        LinkingContext* get_linking_context() { return &linking_context; }

    protected:
        void process_socket(double delta);

        void update_world(double delta);

        void trigger_correct();

        void process_snapshot(StreamReader& reader, WorldSnapshot& snapshot);

        static void _bind_methods();
    };
}

#endif //GODOTPP_NET_MANAGER_H