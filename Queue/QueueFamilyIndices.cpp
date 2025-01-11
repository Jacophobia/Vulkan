#include "QueueFamilyIndices.h"

bool QueueFamilyIndices::is_complete() const
{
    return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
}
