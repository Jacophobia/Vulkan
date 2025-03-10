#include <exception>
#include <iostream>
#include <chrono>
#include <vector>
#include <glm/ext/matrix_transform.hpp>

#include "Graphics/GraphicsRunner.h"
#include "Models/ModelLoading.h"

int main() {
    try
    {
        Camera camera;
        std::function get_target = []{ return glm::vec3(0,0,0); };
        camera.set_target(get_target);
        
        GraphicsRunner app(&camera);
        app.init();

        const std::string model_path = "Models/viking_room.obj";
        const std::string texture_path = "Textures/viking_room.png";

        uint32_t viking_room = app.register_resource({model_path, texture_path, glm::mat4(1.0f)});
        
        uint32_t frame_counter = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto prev_time = std::chrono::high_resolution_clock::now();

        float angle = 0;
        glm::vec3 position(0.f,0.f,0.f);

        while (!app.done())
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time = std::chrono::duration<float>(current_time - prev_time).count();
            
            app.update();
    
            ++frame_counter;
            
            camera.move({0, 0.5f * delta_time, 0});

            angle += 22.5f * delta_time;

            position.x += 0.5f * delta_time;

            app.update_resource(viking_room, rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
            app.update_resource(viking_room, translate(glm::mat4(1.0f), position));

            prev_time = current_time;
        }
        
        app.clean_up();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
