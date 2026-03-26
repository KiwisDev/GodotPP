#ifndef GODOTPP_PLAYER_CONTROLLER_H
#define GODOTPP_PLAYER_CONTROLLER_H

#include <deque>
#include <net_manager.h>
#include <net_protocol.h>
#include <godot_cpp/classes/node.hpp>

namespace godot
{
    class PlayerController : public Node
    {
        GDCLASS(PlayerController, Node)

    private:
        NetworkManager* net_manager = nullptr;

        uint32_t current_sequence = 0;
        std::deque<InputData> input_history;

    public:
        PlayerController();
        ~PlayerController();

        void _ready() override;

        void _physics_process(double p_delta) override;

        void send_input_packet();

    protected:
        static void _bind_methods();
    };
}

#endif //GODOTPP_PLAYER_CONTROLLER_H