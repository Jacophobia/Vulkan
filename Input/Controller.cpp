#include "Controller.h"

#include <iostream>

bool Controller::is_connected() const
{
    return joystick_id_ != -1 && glfwJoystickPresent(joystick_id_) && glfwJoystickIsGamepad(joystick_id_);
}

void Controller::connect()
{
    if (is_connected())
        return;

    // Try to connect to any available gamepad.
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
    {
        if (glfwJoystickPresent(jid) && glfwJoystickIsGamepad(jid))
        {
            joystick_id_ = jid;
            const char* name = glfwGetGamepadName(jid);
            
            std::cout << "Controller connected: " << name << '\n';
            break;
        }
    }
}

void Controller::disconnect()
{
    if (is_connected())
    {
        const char* name = glfwGetGamepadName(joystick_id_);
        std::cout << "Controller disconnected: " << name << '\n';
    }
    joystick_id_ = -1;
}

void Controller::update()
{
    if (!is_connected())
        return;

    GLFWgamepadstate state;
    if (glfwGetGamepadState(joystick_id_, &state))
    {
        // Update button states (15 buttons available).
        for (int i = 0; i < 15; ++i)
        {
            button_states_[i] = state.buttons[i] == GLFW_PRESS;
        }
        
        // Update axis states (6 axes available).
        for (int i = 0; i < 6; ++i)
        {
            axis_states_[i] = state.axes[i];
        }
    }
}

void Controller::add_button_mapping(Control control, int button_id)
{
    button_control_mappings_[control] = button_id;
}

void Controller::add_axis_mapping(Control control, int axis_id, float activation_threshold, bool positive_and_negative)
{
    axis_control_mappings_[control] = std::make_tuple(activation_threshold, axis_id, positive_and_negative);
}

float Controller::get_control_state(Control control) const
{
    // Check if there is an axis mapping.
    if (axis_control_mappings_.contains(control))
    {
        auto [activation_threshold, axis_id, positive_and_negative] = axis_control_mappings_.at(control);
        if (axis_id < static_cast<int>(axis_states_.size()))
        {
            float axis_state = positive_and_negative
                ? abs(axis_states_[axis_id])
                : axis_states_[axis_id];

            if (axis_state >= activation_threshold)
            {
                return axis_states_[axis_id];
            }
        }
    }

    // Otherwise, check button mapping.
    if (button_control_mappings_.contains(control))
    {
        int button_id = button_control_mappings_.at(control);
        if (button_id < static_cast<int>(button_states_.size()) && button_states_[button_id])
        {
            return 1.f;
        }
    }

    return 0.f;
}

bool Controller::was_connection_interrupted() const
{
    return joystick_id_ != -1 && !glfwJoystickPresent(joystick_id_);
}