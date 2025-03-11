#pragma once

#include "Controls.h"
#include <array>
#include <unordered_map>
#include <tuple>
#include <GLFW/glfw3.h>

class Controller
{
public:
    // Returns true if a gamepad is connected and recognized by GLFW.
    [[nodiscard]] bool is_connected() const;
    
    // Attempts to connect to the first available gamepad.
    void connect();
    
    // Disconnects the current gamepad.
    void disconnect();
    
    // Polls the current gamepad state and updates button/axis states.
    void update();
    
    // Map a control string to a gamepad button id.
    void add_button_mapping(Control control, int button_id);
    
    // Map a control string to an axis, with a given activation threshold.
    void add_axis_mapping(Control control, int axis_id, float activation_threshold, bool positive_and_negative);
    
    // Returns the control state: for axes, if the axis value exceeds the threshold, returns a scaled integer value;
    // for buttons, returns 1 if pressed.
    float get_control_state(Control control) const;

private:
    // GLFW gamepad state: gamepads have 15 buttons and 6 axes.
    std::array<bool, 15> button_states_{};  
    std::array<float, 6> axis_states_{};

    // Mappings from control strings to GLFW gamepad button or axis indices.
    std::unordered_map<Control, int> button_control_mappings_{};
    std::unordered_map<Control, std::tuple<float, int, bool>> axis_control_mappings_{};

    // The joystick id for the connected gamepad; -1 if none.
    int joystick_id_ = -1;

    bool was_connection_interrupted() const;
};
