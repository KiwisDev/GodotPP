#include "player_controller.h"

#include <net_manager.h>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

godot::PlayerController::PlayerController() {}

godot::PlayerController::~PlayerController() {}

void PlayerController::_ready()
{
    Node::_ready();

    Node* found_node = get_tree()->get_first_node_in_group("NetworkManager");

    if (found_node)
    {
        net_manager = Object::cast_to<NetworkManager>(found_node);
        UtilityFunctions::print("[PLAYERCONTROLLER] NetworkManager found");
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

    //uint8_t* raw_data = reinterpret_cast<uint8_t*>(&packet);
    //size_t raw_data_size = sizeof(packet);

    net_manager->send_packet(packet_data.data(), packet_data.size());
}

void godot::PlayerController::_bind_methods() {}
