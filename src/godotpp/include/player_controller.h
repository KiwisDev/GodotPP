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

        float speed = 150.0f;

        uint32_t current_sequence = 0;
        std::deque<InputData> input_history;

        Vector2 visual_velocity = Vector2(0, 0);
        Vector2 logical_position = Vector2(0, 0);

        float spring_stiffness = 800.0f;
        float spring_damping = 30.0f;
        float snap_threshold = 200.0f;

    public:
        PlayerController();
        ~PlayerController();

        void _ready() override;

        void _physics_process(double p_delta) override;

        void send_input_packet();

        void correct_movement(Vector2 server_pos, uint32_t last_processed_sequence);

        void set_spring_stiffness(float p_stiffness) { spring_stiffness = p_stiffness; }
        float get_spring_stiffness() const { return spring_stiffness; }

        void set_spring_damping(float p_damping) { spring_damping = p_damping; }
        float get_spring_damping() const { return spring_damping; }

        void set_snap_threshold(float p_threshold) { snap_threshold = p_threshold; }
        float get_snap_threshold() const { return snap_threshold; }

        void set_accept_threshold(float p_threshold) { accept_threshold = p_threshold; }
        float get_accept_threshold() const { return accept_threshold; }

    protected:
        void simulate_movement(const InputData& input);

        Vector2 calculate_velocity(const InputData& input);

        static void _bind_methods();
    };
}

#endif //GODOTPP_PLAYER_CONTROLLER_H