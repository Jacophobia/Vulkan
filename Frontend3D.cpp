
#include <exception>
#include <iostream>

#include "Triangle/GraphicsRunner.h"

int main() {
    try
    {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
