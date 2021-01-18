#include "sys/vk/commands.hpp"

#include "base/types.hpp"

namespace minote::sys::vk {

using namespace base;

void cmdSetArea(VkCommandBuffer cmdBuf, VkExtent2D size) {
	auto const viewport = VkViewport{
		.width = static_cast<f32>(size.width),
		.height = static_cast<f32>(size.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	auto const scissor = VkRect2D{
		.extent = size,
	};
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

}
