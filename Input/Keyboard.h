#pragma once

#include "Controls.h"
#include <unordered_map>
#include <GLFW/glfw3.h>

class Keyboard
{
public:
    // Polls the state of keys for the mapped controls.
    void update(GLFWwindow* window);
    
    // Adds a mapping between a control string and a GLFW key code.
    void add_mapping(Control control, int key);
    
    // Returns 1 if the mapped key for the given control is pressed.
    float get_control_state(Control control) const;

private:
    // Maps control strings to GLFW key codes.
    std::unordered_map<Control, int> control_mappings_;
    
    // Stores the current state for each key (true if pressed).
    std::unordered_map<int, bool> key_states_;
};