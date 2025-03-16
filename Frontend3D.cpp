#include <exception>
#include <iostream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

#include "Actors/Actor.h"
#include "Graphics/GraphicsRunner.h"
#include "Models/ModelLoading.h"
#include "Input/Input.h"

void initialize_inputs(Input& input)
{
    input.create(true);

    // Set up control mappings:

    // Movement controls:
    // For controller: use left joystick x-axis (axis 0) for MOVE_RIGHT and left joystick y-axis (axis 1) for MOVE_UP.
    // For keyboard: map MOVE_RIGHT to the D key and MOVE_UP to the W key.
    input.add_axis_mapping(controls::MOVE_RIGHT, 0, 0.1f);
    input.add_key_mapping(controls::MOVE_RIGHT, GLFW_KEY_D);
    input.add_axis_mapping(controls::MOVE_UP, 1, 0.1f);
    input.add_key_mapping(controls::MOVE_UP, GLFW_KEY_W);

    // Look controls:
    // For controller: use right joystick x-axis (axis 2) and y-axis (axis 3).
    // (No keyboard mapping provided here because mouse movement isn't polled via key mapping.)
    input.add_axis_mapping(controls::LOOK_RIGHT, 2, 0.1f);
    input.add_axis_mapping(controls::LOOK_UP, 3, 0.1f);

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
    input.add_axis_mapping(controls::ABILITY_SIX, 4, 0.1f, false);

    input.add_key_mapping(controls::ABILITY_SEVEN, GLFW_KEY_7);
    input.add_button_mapping(controls::ABILITY_SEVEN, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);

    input.add_key_mapping(controls::ABILITY_EIGHT, GLFW_KEY_8);
    input.add_axis_mapping(controls::ABILITY_EIGHT, 5, 0.1f, false);

    input.add_key_mapping(controls::ABILITY_NINE, GLFW_KEY_9);
    input.add_button_mapping(controls::ABILITY_NINE, GLFW_GAMEPAD_BUTTON_LEFT_THUMB);

    input.add_key_mapping(controls::ABILITY_TEN, GLFW_KEY_0);
    input.add_button_mapping(controls::ABILITY_TEN, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);

    // Pause and Select:
    input.add_key_mapping(controls::PAUSE, GLFW_KEY_ESCAPE);
    input.add_button_mapping(controls::PAUSE, GLFW_GAMEPAD_BUTTON_START);

    input.add_key_mapping(controls::SELECT, GLFW_KEY_M);
    input.add_button_mapping(controls::SELECT, GLFW_GAMEPAD_BUTTON_BACK);
}

void debug_controller_inputs(Input& input)
{
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
}

int main() {
    try
    {
        // TestInput::run();
        // return 0; 
        
        Camera camera;
        camera.move({0,0,0});
        
        GraphicsRunner app(&camera);
        app.init();

        const std::string sphere_model_path = "Models/sphere.obj";
        const std::string sphere_texture_path = "Textures/grid.jpg";
        
        const std::string cube_model_path = "Models/cube.obj";
        const std::string cube_texture_path = "Textures/grid.jpg";

        glm::vec3 world_up(0.f, 0.f, 1.f);
        
        uint32_t earth_id = app.register_resource({sphere_model_path, sphere_texture_path, glm::mat4(1.0f)});
        uint32_t cube_id = app.register_resource({cube_model_path, cube_texture_path, glm::mat4(1.0f)});
        uint32_t moon_id = app.register_resource({sphere_model_path, sphere_texture_path, glm::mat4(1.f)});
        Actor cube(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.001f, 0.001f, 0.001f), world_up, 0.f, 0.f, 0.f);
        Actor earth({0.f, 384399.f / 2.f, 0.f}, {6378.f, 6378.f, 6378.f}, world_up, 0.f, 0.f, 0.f);
        Actor moon({0.f, -384399.f / 2.f, 0.f}, {1738.f, 1738.f, 1738.f}, world_up, 0.f, 0.f, 0.f);
        
        uint32_t frame_counter = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto prev_time = std::chrono::high_resolution_clock::now();
        
        // std::function get_target = [&position]{ return position; };
        // std::function get_target = []{ return glm::vec3(0,0,0); };
        // camera.set_target(get_target);
        
        Input input;
        initialize_inputs(input);

        while (!app.done())
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time = std::chrono::duration<float>(current_time - prev_time).count();
            
            app.update();
            input.update(app.window_); // TODO: architect this better
    
            ++frame_counter;

            // debug_controller_inputs(input);

            if (input.get_control_state(controls::MOVE_RIGHT) != 0.f || input.get_control_state(controls::MOVE_UP) != 0.f)
            {
                float x = input.get_control_state(controls::MOVE_RIGHT);
                float y = input.get_control_state(controls::MOVE_UP) * -1;
                
                camera.travel(x, y, 0.f, delta_time);
            }
            if (input.get_control_state(controls::LOOK_RIGHT) != 0.f || input.get_control_state(controls::LOOK_UP) != 0.f)
            {
                float x = input.get_control_state(controls::LOOK_RIGHT);
                float y = input.get_control_state(controls::LOOK_UP) * -1;
                
                camera.look(x, y, delta_time);
            }
            if (input.get_control_state(controls::ABILITY_ONE) != 0.f)
            {
                camera.travel(0.f, 0.f, -input.get_control_state(controls::ABILITY_ONE), delta_time);
            }
            if (input.get_control_state(controls::ABILITY_TWO) != 0.f)
            {
                camera.travel(0.f, 0.f, input.get_control_state(controls::ABILITY_TWO), delta_time);
            }

            // cube.move(glm::vec3(-0.5f * delta_time, 0.5f * delta_time, 0));
            cube.roll(10.f * delta_time);
            cube.pitch(-15.f * delta_time);
            cube.yaw(1.f * delta_time);

            earth.roll(1.f * delta_time);
            earth.pitch(1.f * delta_time);
            earth.pitch(1.f * delta_time);
            // earth.move({0.f, -1.f, 0.f});
            
            app.update_resource(earth_id, earth.get_transform());

            app.update_resource(cube_id, cube.get_transform());
            app.update_resource(moon_id, moon.get_transform());

            prev_time = current_time;
        }
        
        input.destroy();
        app.clean_up();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
