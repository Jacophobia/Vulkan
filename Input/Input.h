#pragma once

#include "Controller.h"
#include "Keyboard.h"
#include "Controls.h"
#include <GLFW/glfw3.h>

class Input
{
public:
    // Initializes input, optionally using a gamepad.
    void create(bool use_controller = true);
    
    // Connects the controller if needed.
    void connect();
    
    // Polls the current input states from both controller and keyboard.
    // (Pass the GLFWwindow pointer for keyboard polling.)
    void update(GLFWwindow* window);
    
    // Disconnects and cleans up input devices.
    void destroy();
    
    // Adds a mapping for a gamepad button.
    void add_button_mapping(Control control, int button_id);
    
    // Adds a mapping for a gamepad axis with a given activation threshold.
    void add_axis_mapping(Control control, int axis_id, float activation_threshold, bool positive_and_negative = true);
    
    // Adds a mapping for a keyboard key (using GLFW key codes).
    void add_key_mapping(Control control, int key);
    
    // Retrieves the control state from both controller and keyboard (returns the maximum value).
    float get_control_state(Control control) const;

private:
    Controller controller_;
    Keyboard keyboard_;
    bool use_controller_ = true;
};