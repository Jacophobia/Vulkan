#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texture_coordinate;

    static VkVertexInputBindingDescription get_binding_description();
    static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions();

    bool operator==(const Vertex & rhs) const;
};

template<>
struct std::hash<Vertex>
{
    size_t operator()(Vertex const& vertex) const noexcept;
};
