#include <exception>
#include <iostream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

#include "Graphics/GraphicsRunner.h"
#include "Models/ModelLoading.h"
#include "Input/Input.h"
#include "Tests/TestInput.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

void initialize_physics()
{
    // Initialize Jolt Physics once at startup
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();

    // Initialize Jolt Physics types
    JPH::RegisterTypes();

    // Job System for parallel execution (optional but recommended)
    JPH::JobSystemThreadPool job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency());
}

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
        camera.move({0,0,1});
        
        GraphicsRunner app(&camera);
        app.init();

        const std::string model_path = "Models/viking_room.obj";
        const std::string texture_path = "Textures/viking_room.png";

        uint32_t viking_room_1 = app.register_resource({model_path, texture_path, glm::mat4(1.0f)});
        uint32_t viking_room_2 = app.register_resource({model_path, texture_path, glm::mat4(1.0f)});
        
        uint32_t frame_counter = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto prev_time = std::chrono::high_resolution_clock::now();

        float angle = 0;
        glm::vec3 position(0.f,0.f,0.f);
        
        // std::function get_target = [&position]{ return position; };
        // // std::function get_target = []{ return glm::vec3(0,0,0); };
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

            debug_controller_inputs(input);

            if (input.get_control_state(controls::MOVE_RIGHT) != 0.f || input.get_control_state(controls::MOVE_UP) != 0.f)
            {
                float x = input.get_control_state(controls::MOVE_RIGHT);
                float y = input.get_control_state(controls::MOVE_UP) * -1;
                
                camera.travel(x, y, delta_time);
            }
            if (input.get_control_state(controls::LOOK_RIGHT) != 0.f || input.get_control_state(controls::LOOK_UP) != 0.f)
            {
                float x = input.get_control_state(controls::LOOK_RIGHT);
                float y = input.get_control_state(controls::LOOK_UP) * -1;
                
                camera.look(x, y, delta_time);
            }
            
            // camera.move({0, 0, 0.75f * delta_time});

            angle += 90.f * delta_time;

            position.x += 0.5f * delta_time;

            glm::mat4 matrix_1(1.0f);
            // matrix_1 = translate(matrix_1, position);
            // matrix_1 = rotate(matrix_1, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
            
            app.update_resource(viking_room_1, matrix_1);

            glm::mat4 matrix_2(1.0f);
            matrix_2 = rotate(matrix_2, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
            matrix_2 = translate(matrix_2, position);
            
            app.update_resource(viking_room_2, matrix_2);

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
