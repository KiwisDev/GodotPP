#include "player_controller.h"

#include <net_manager.h>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "godot_cpp/classes/node2d.hpp"

godot::PlayerController::PlayerController() {}

godot::PlayerController::~PlayerController() {}

void PlayerController::_ready()
{
    Node::_ready();

    Node* found_node = get_tree()->get_first_node_in_group("NetworkManager");

    if (found_node)
    {
        net_manager = Object::cast_to<NetworkManager>(found_node);
        UtilityFunctions::print("[PlayerController] NetworkManager found");
    }
}

void godot::PlayerController::_physics_process(double p_delta)
{
    Node::_physics_process(p_delta);

    Input* input = Input::get_singleton();

    uint8_t keys = 0;
    if (input->is_action_pressed("move_up")) keys |= FLAG_UP;
    if (input->is_action_pressed("move_down")) keys |= FLAG_DOWN;
    if (input->is_action_pressed("move_left")) keys |= FLAG_LEFT;
    if (input->is_action_pressed("move_right")) keys |= FLAG_RIGHT;
    if (input->is_action_pressed("action")) keys |= FLAG_ACTION;

    Vector2 mouse_pos = input->get_last_mouse_velocity();

    InputData current_input;
    current_input.sequence = ++current_sequence;
    current_input.keys = keys;
    current_input.aim_x = mouse_pos.x;
    current_input.aim_y = mouse_pos.y;

    input_history.push_back(current_input);
    if (input_history.size() > 20)
    {
        input_history.pop_front();
    }

    if (net_manager != nullptr)
    {
        send_input_packet();
    }

    simulate_movement(current_input);

    Node* player_node = net_manager->get_linking_context()->get_node(net_manager->get_local_player_net_id());
    if (player_node)
    {
        Node2D* node2d = dynamic_cast<Node2D*>(player_node);

        Vector2 visual_pos = node2d->get_global_position();
        Vector2 diff = logical_position - visual_pos;

        if (diff.length() > snap_threshold)
        {
            node2d->set_global_position(logical_position);
            visual_velocity = Vector2(0, 0);
        }
        else
        {
            Vector2 spring_force = diff * spring_stiffness;
            Vector2 damping_force = visual_velocity * spring_damping;
            Vector2 accel = spring_force - damping_force;

            visual_velocity += accel * p_delta;
            Vector2 new_visual_pos = visual_pos + (visual_velocity * p_delta);

            node2d->set_global_position(new_visual_pos);
        }
    }
}

void godot::PlayerController::send_input_packet()
{
    InputPacket packet;
    packet.type = PacketType::INPUT;
    memset(&packet.input_history, 0, sizeof(packet.input_history));

    int start_idx = 20 - input_history.size();
    for (int i=0; i < input_history.size(); ++i)
    {
        packet.input_history[start_idx + i] = input_history[i];
    }

    StreamWriter w;
    packet.serialize(w);
    std::vector<uint8_t> packet_data = w.finish();

    net_manager->send_packet(packet_data.data(), packet_data.size());
}

void PlayerController::simulate_movement(const InputData& input)
{
    if (!net_manager) return;

    Node* player_node = net_manager->get_linking_context()->get_node(net_manager->get_local_player_net_id());
    if (!player_node) return;

    Node2D* node2d = dynamic_cast<Node2D*>(player_node);

    Vector2 movement = calculate_velocity(input) * speed * 1.0f/60.0f;
    logical_position += movement;
    Vector2 new_pos = node2d->get_global_position() + movement;
    node2d->set_position(new_pos);
}

void PlayerController::correct_movement(Vector2 server_pos, uint32_t last_processed_sequence)
{
    if (!net_manager) return;

    Node* player_node = net_manager->get_linking_context()->get_node(net_manager->get_local_player_net_id());
    if (!player_node) return;

    Node2D* node2d = dynamic_cast<Node2D*>(player_node);

    std::deque<InputData> temp_input_history(input_history);

    while (!temp_input_history.empty() && temp_input_history.front().sequence <= last_processed_sequence)
    {
        temp_input_history.pop_front();
    }

    logical_position = server_pos;
    for (const InputData& pending_input : temp_input_history)
    {
        logical_position += calculate_velocity(pending_input) * speed * 1.0f/60.0f;
    }
}

Vector2 PlayerController::calculate_velocity(const InputData& input)
{
    Vector2 velocity(0, 0);

    if (input.keys & FLAG_UP) velocity.y -= 1;
    if (input.keys & FLAG_DOWN) velocity.y += 1;
    if (input.keys & FLAG_LEFT) velocity.x -= 1;
    if (input.keys & FLAG_RIGHT) velocity.x += 1;

    if (velocity.length_squared() > 0) {
        velocity = velocity.normalized();
    }

    return velocity;
}

void godot::PlayerController::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_spring_stiffness", "stiffness"), &PlayerController::set_spring_stiffness);
    ClassDB::bind_method(D_METHOD("get_spring_stiffness"), &PlayerController::get_spring_stiffness);
    ClassDB::add_property("PlayerController", PropertyInfo(Variant::FLOAT, "spring_stiffness"), "set_spring_stiffness", "get_spring_stiffness");

    ClassDB::bind_method(D_METHOD("set_spring_damping", "damping"), &PlayerController::set_spring_damping);
    ClassDB::bind_method(D_METHOD("get_spring_damping"), &PlayerController::get_spring_damping);
    ClassDB::add_property("PlayerController", PropertyInfo(Variant::FLOAT, "spring_damping"), "set_spring_damping", "get_spring_damping");

    ClassDB::bind_method(D_METHOD("set_snap_threshold", "snap_threshold"), &PlayerController::set_snap_threshold);
    ClassDB::bind_method(D_METHOD("get_snap_threshold"), &PlayerController::get_snap_threshold);
    ClassDB::add_property("PlayerController", PropertyInfo(Variant::FLOAT, "snap_threshold"), "set_snap_threshold", "get_snap_threshold");
}
