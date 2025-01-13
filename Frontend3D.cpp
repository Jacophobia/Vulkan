
#include <exception>
#include <iostream>

#include "Graphics/GraphicsRunner.h"

int main() {
    try
    {
        GraphicsRunner app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
