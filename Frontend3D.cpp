#include <exception>
#include <iostream>
#include <chrono>
#include <vector>

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

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        const std::string model_path = "Models/viking_room.obj";
        model_loading::load_model(vertices, indices, model_path);

        auto viking_room = app.register_resource({vertices, indices, {}});
        
        uint32_t frame_counter = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto prev_time = std::chrono::high_resolution_clock::now();

        while (!app.done())
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time = std::chrono::duration<float>(current_time - prev_time).count();
            
            app.update();
    
            ++frame_counter;
            
            camera.move({0, 0.5f * delta_time, 0});

            prev_time = current_time;
        }
        
        app.clean_up();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
