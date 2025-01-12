#include "Vertex.h"

VkVertexInputBindingDescription Vertex::get_binding_description()
{
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::get_attribute_descriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, pos);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinate);
    
    return attribute_descriptions;
}

bool Vertex::operator==(const Vertex &rhs) const
{
    return pos == rhs.pos && color == rhs.color && texture_coordinate == rhs.texture_coordinate;
}

size_t std::hash<Vertex>::operator()(Vertex const &vertex) const noexcept
{
    constexpr std::hash<float> hasher;

    // Hash each component of the color vector
    size_t h1 = hasher(vertex.color.x);
    h1 ^= hasher(vertex.color.y) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
    h1 ^= hasher(vertex.color.z) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);

    // Hash each component of the position vector
    size_t h2 = hasher(vertex.pos.x);
    h2 ^= hasher(vertex.pos.y) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
    h2 ^= hasher(vertex.pos.z) + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);

    // Hash each component of the texture_coordinate vector
    size_t h3 = hasher(vertex.texture_coordinate.x);
    h3 ^= hasher(vertex.texture_coordinate.y) + 0x9e3779b9 + (h3 << 6) + (h3 >> 2);

    // Combine all hashes
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}
