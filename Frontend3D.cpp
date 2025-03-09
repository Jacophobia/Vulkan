#include <exception>
#include <iostream>
#include <chrono>

#include "Graphics/GraphicsRunner.h"
#include "Logging/Logging.h"

int main() {
    try
    {
        Camera camera;
        camera.move(glm::vec3(-1,-1,-2));
        camera.set_target({0,0,0});
        
        GraphicsRunner app(&camera);
        app.init();
        
        uint32_t frame_counter = 0;
        auto start_time = std::chrono::high_resolution_clock::now();

        while (!app.done())
        {
            app.update();
    
            ++frame_counter;

            camera.set_position({frame_counter % 100000 / 1000.f, frame_counter % 100000 / 1000.f, frame_counter % 100000 / 1000.f});
    
            auto current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<float>(current_time - start_time).count() >= 1.0f)
            {
                logging::info(std::format("FPS: {}", frame_counter));
        
                start_time = std::chrono::high_resolution_clock::now();
                frame_counter = 0;
            }
        }
        
        app.clean_up();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
