#include "Keyboard.h"

#include <ranges>

void Keyboard::update(GLFWwindow* window)
{
    // For each mapped key, poll its state and update.
    for (const auto &key : control_mappings_ | std::views::values)
    {
        const int state = glfwGetKey(window, key);
        const bool pressed = (state == GLFW_PRESS);

        // Check if the key state has changed and print a message.
        if (key_states_[key] != pressed)
        {
            const char* key_name = glfwGetKeyName(key, 0);
            key_states_[key] = pressed;
        }
    }
}

float Keyboard::get_control_state(const Control control) const
{
    if (control_mappings_.contains(control))
    {
        const int key = control_mappings_.at(control);
        auto it = key_states_.find(key);
        if (key_states_.contains(key) && key_states_.at(key))
            return 1.f;
    }
    return 0.f;
}

void Keyboard::add_mapping(const Control control, const int key)
{
    control_mappings_[control] = key;
}