#pragma once

#include "volk/volk.h"

namespace minote::sys::vk {

void cmdSetArea(VkCommandBuffer cmdBuf, VkExtent2D size);

}
