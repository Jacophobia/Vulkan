#pragma once

#include <cstdint>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    std::optional<uint32_t> transfer_family;

    [[nodiscard]] bool is_complete() const;
};
