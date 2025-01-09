#include "Logging.h"

#include <iostream>

#define LOG_ERRORS
#define LOG_WARNINGS
#define LOG_INFO

void logging::error(std::string message)
{
#ifdef LOG_ERRORS
    std::cerr << "Error: " << message << "\n";
#endif
}

void logging::warning(std::string message)
{
#ifdef LOG_WARNINGS
    std::cout << "Warning: " << message << "\n";
#endif
}

void logging::info(std::string message)
{
#ifdef LOG_INFO
    std::cout << "Info: " << message << "\n";
#endif
}
