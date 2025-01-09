#include "SwapChainSupportDetails.h"

bool SwapChainSupportDetails::is_complete() const
{
    return !formats.empty() && ! present_modes.empty();
}
