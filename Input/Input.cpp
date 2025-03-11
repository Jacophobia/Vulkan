#include "Input.h"
#include <algorithm>

void Input::create(const bool use_controller)
{
    use_controller_ = use_controller;
    if (use_controller_)
    {
        connect();
    }
}

void Input::connect()
{
    if (!use_controller_ || controller_.is_connected())
        return;
    
    controller_.connect();
}

void Input::update(GLFWwindow* window)
{
    if (use_controller_)
    {
        controller_.update();
    }
    keyboard_.update(window);
}

void Input::destroy()
{
    controller_.disconnect();
}

void Input::add_button_mapping(Control control, int button_id)
{
    controller_.add_button_mapping(control, button_id);
}

void Input::add_axis_mapping(Control control, int axis_id, float activation_threshold, bool positive_and_negative)
{
    controller_.add_axis_mapping(control, axis_id, activation_threshold, positive_and_negative);
}

void Input::add_key_mapping(Control control, int key)
{
    keyboard_.add_mapping(control, key);
}

float Input::get_control_state(Control control) const
{
    const float controller_state = controller_.get_control_state(control);
    const float keyboard_state = keyboard_.get_control_state(control);
    if (std::abs(controller_state) > std::abs(keyboard_state))
    {
        return controller_state;
    }
    return keyboard_state;
}
