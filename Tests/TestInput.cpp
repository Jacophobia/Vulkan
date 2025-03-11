#include "TestInput.h"

#include "../Input/Input.h"
#include "../Input/Controls.h"
#include <GLFW/glfw3.h>
#include <iostream>

void TestInput::run()
{
    manual_tests();
}

void TestInput::manual_tests()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed" << '\n';
        return;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Input Test", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << '\n';
        glfwTerminate();
        return;
    }
    // glfwMakeContextCurrent(window);

    // Create and initialize the Input system.
    // 'true' indicates we want to use a controller (if available) as well as keyboard.
    Input input;
    input.create(true);

    // Set up control mappings:

    // Movement controls:
    // For controller: use left joystick x-axis (axis 0) for MOVE_RIGHT and left joystick y-axis (axis 1) for MOVE_UP.
    // For keyboard: map MOVE_RIGHT to the D key and MOVE_UP to the W key.
    input.add_axis_mapping(controls::MOVE_RIGHT, 0, 0.5f);
    input.add_key_mapping(controls::MOVE_RIGHT, GLFW_KEY_D);
    input.add_axis_mapping(controls::MOVE_UP, 1, 0.5f);
    input.add_key_mapping(controls::MOVE_UP, GLFW_KEY_W);

    // Look controls:
    // For controller: use right joystick x-axis (axis 2) and y-axis (axis 3).
    // (No keyboard mapping provided here because mouse movement isn't polled via key mapping.)
    input.add_axis_mapping(controls::LOOK_RIGHT, 2, 0.5f);
    input.add_axis_mapping(controls::LOOK_UP, 3, 0.5f);

    // Ability controls:
    input.add_key_mapping(controls::ABILITY_ONE, GLFW_KEY_1);
    input.add_button_mapping(controls::ABILITY_ONE, GLFW_GAMEPAD_BUTTON_A);

    input.add_key_mapping(controls::ABILITY_TWO, GLFW_KEY_2);
    input.add_button_mapping(controls::ABILITY_TWO, GLFW_GAMEPAD_BUTTON_B);

    input.add_key_mapping(controls::ABILITY_THREE, GLFW_KEY_3);
    input.add_button_mapping(controls::ABILITY_THREE, GLFW_GAMEPAD_BUTTON_X);

    input.add_key_mapping(controls::ABILITY_FOUR, GLFW_KEY_4);
    input.add_button_mapping(controls::ABILITY_FOUR, GLFW_GAMEPAD_BUTTON_Y);

    input.add_key_mapping(controls::ABILITY_FIVE, GLFW_KEY_5);
    input.add_button_mapping(controls::ABILITY_FIVE, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);

    input.add_key_mapping(controls::ABILITY_SIX, GLFW_KEY_6);
    input.add_axis_mapping(controls::ABILITY_SIX, 4, 0.5, false);

    input.add_key_mapping(controls::ABILITY_SEVEN, GLFW_KEY_7);
    input.add_button_mapping(controls::ABILITY_SEVEN, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);

    input.add_key_mapping(controls::ABILITY_EIGHT, GLFW_KEY_8);
    input.add_axis_mapping(controls::ABILITY_EIGHT, 5, 0.5, false);

    input.add_key_mapping(controls::ABILITY_NINE, GLFW_KEY_9);
    input.add_button_mapping(controls::ABILITY_NINE, GLFW_GAMEPAD_BUTTON_LEFT_THUMB);

    input.add_key_mapping(controls::ABILITY_TEN, GLFW_KEY_0);
    input.add_button_mapping(controls::ABILITY_TEN, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);

    // Pause and Select:
    input.add_key_mapping(controls::PAUSE, GLFW_KEY_ESCAPE);
    input.add_button_mapping(controls::PAUSE, GLFW_GAMEPAD_BUTTON_START);

    input.add_key_mapping(controls::SELECT, GLFW_KEY_M);
    input.add_button_mapping(controls::SELECT, GLFW_GAMEPAD_BUTTON_BACK);

    // Print instructions
    std::cout << "Controls mapping:" << '\n';
    std::cout << "MOVE_RIGHT: Keyboard D or Left Stick X-axis" << '\n';
    std::cout << "MOVE_UP: Keyboard W or Left Stick Y-axis" << '\n';
    std::cout << "LOOK_RIGHT: Right Stick X-axis" << '\n';
    std::cout << "LOOK_UP: Right Stick Y-axis" << '\n';
    std::cout << "ABILITY_ONE: Keyboard 1 or Controller A" << '\n';
    std::cout << "ABILITY_TWO: Keyboard 2 or Controller B" << '\n';
    std::cout << "ABILITY_THREE: Keyboard 3 or Controller X" << '\n';
    std::cout << "ABILITY_FOUR: Keyboard 4 or Controller Y" << '\n';
    std::cout << "ABILITY_FIVE: Keyboard 5 or Left Bumper" << '\n';
    std::cout << "ABILITY_SIX: Keyboard 6 or Left Trigger" << '\n';
    std::cout << "ABILITY_SEVEN: Keyboard 7 or Right Bumper" << '\n';
    std::cout << "ABILITY_EIGHT: Keyboard 8 or Right Trigger" << '\n';
    std::cout << "ABILITY_NINE: Keyboard 9 or Left Joystick Button" << '\n';
    std::cout << "ABILITY_TEN: Keyboard 0 or Right Joystick Button" << '\n';
    std::cout << "PAUSE: Keyboard Escape or Controller Start" << '\n';
    std::cout << "SELECT: Keyboard M or Controller Back" << '\n';
    std::cout << "Press ESC to exit." << '\n' << '\n';

    // Main loop: run until the window is closed or ESC is pressed.
    while (!glfwWindowShouldClose(window))
    {
        // Poll GLFW events.
        glfwPollEvents();

        // Check for exit request.
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        // Update our Input system.
        input.update(window);

        // Retrieve and print active control states.
        if (input.get_control_state(controls::MOVE_RIGHT))
            std::cout << "MOVE_RIGHT active: " << input.get_control_state(controls::MOVE_RIGHT) << '\n';
        if (input.get_control_state(controls::MOVE_UP))
            std::cout << "MOVE_UP active: " << input.get_control_state(controls::MOVE_UP) << '\n';
        if (input.get_control_state(controls::LOOK_RIGHT))
            std::cout << "LOOK_RIGHT active: " << input.get_control_state(controls::LOOK_RIGHT) << '\n';
        if (input.get_control_state(controls::LOOK_UP))
            std::cout << "LOOK_UP active: " << input.get_control_state(controls::LOOK_UP) << '\n';
        if (input.get_control_state(controls::ABILITY_ONE))
            std::cout << "ABILITY_ONE active: " << input.get_control_state(controls::ABILITY_ONE) << '\n';
        if (input.get_control_state(controls::ABILITY_TWO))
            std::cout << "ABILITY_TWO active: " << input.get_control_state(controls::ABILITY_TWO) << '\n';
        if (input.get_control_state(controls::ABILITY_THREE))
            std::cout << "ABILITY_THREE active: " << input.get_control_state(controls::ABILITY_THREE) << '\n';
        if (input.get_control_state(controls::ABILITY_FOUR))
            std::cout << "ABILITY_FOUR active: " << input.get_control_state(controls::ABILITY_FOUR) << '\n';
        if (input.get_control_state(controls::ABILITY_FIVE))
            std::cout << "ABILITY_FIVE active: " << input.get_control_state(controls::ABILITY_FIVE) << '\n';
        if (input.get_control_state(controls::ABILITY_SIX))
            std::cout << "ABILITY_SIX active: " << input.get_control_state(controls::ABILITY_SIX) << '\n';
        if (input.get_control_state(controls::ABILITY_SEVEN))
            std::cout << "ABILITY_SEVEN active: " << input.get_control_state(controls::ABILITY_SEVEN) << '\n';
        if (input.get_control_state(controls::ABILITY_EIGHT))
            std::cout << "ABILITY_EIGHT active: " << input.get_control_state(controls::ABILITY_EIGHT) << '\n';
        if (input.get_control_state(controls::ABILITY_NINE))
            std::cout << "ABILITY_NINE active: " << input.get_control_state(controls::ABILITY_NINE) << '\n';
        if (input.get_control_state(controls::ABILITY_TEN))
            std::cout << "ABILITY_TEN active: " << input.get_control_state(controls::ABILITY_TEN) << '\n';
        if (input.get_control_state(controls::PAUSE))
            std::cout << "PAUSE active: " << input.get_control_state(controls::PAUSE) << '\n';
        if (input.get_control_state(controls::SELECT))
            std::cout << "SELECT active: " << input.get_control_state(controls::SELECT) << '\n';

        // glfwSwapBuffers(window);
    }

    // Clean up.
    input.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}
