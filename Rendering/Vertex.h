#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription get_binding_description();
    static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions();
};

const std::vector<Vertex> test_triangle =
{
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
};
