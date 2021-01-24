#include "gfx/swapchain.hpp"

#include <thread>
#include <limits>
#include "base/zip_view.hpp"
#include "base/time.hpp"
#include "base/util.hpp"
#include "base/log.hpp"
#include "sys/vk/debug.hpp"
#include "gfx/base.hpp"

namespace minote::gfx {

using namespace base;
using namespace base::literals;
namespace vk = sys::vk;

void Swapchain::init(Context& ctx, VkSwapchainKHR old) {
	// Find the best swapchain options
	auto const surfaceFormat = [&]() -> VkSurfaceFormatKHR {
		for (auto const& fmt: ctx.surfaceFormats) {
			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
				fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return fmt;
		}
		return ctx.surfaceFormats[0];
	}();
	auto const surfacePresentMode = [&]() -> VkPresentModeKHR {
		for (auto const mode: ctx.surfacePresentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}();
	while (true) {
		if (ctx.window->isClosing())
			throw std::runtime_error{"Window close detected during a resize/minimized state"};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &ctx.surfaceCapabilities);
		auto const windowSize = ctx.window->size();
		extent = VkExtent2D{
			.width = std::clamp(windowSize.x,
				ctx.surfaceCapabilities.minImageExtent.width,
				ctx.surfaceCapabilities.maxImageExtent.width),
			.height = std::clamp(windowSize.y,
				ctx.surfaceCapabilities.minImageExtent.height,
				ctx.surfaceCapabilities.maxImageExtent.height),
		};
		if (extent.width != 0 && extent.height != 0) break;
		std::this_thread::sleep_for(10_ms);
	}
	auto const maxImageCount = ctx.surfaceCapabilities.maxImageCount?: 256;
	auto const surfaceImageCount = std::min(ctx.surfaceCapabilities.minImageCount + 1, maxImageCount);

	// Create the swapchain
	auto const queueIndices = std::array{ctx.graphicsQueueFamilyIndex, ctx.presentQueueFamilyIndex};
	auto const swapchainCI = VkSwapchainCreateInfoKHR{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx.surface,
		.minImageCount = surfaceImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = ctx.uniquePresentQueue()? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = static_cast<u32>(ctx.uniquePresentQueue()? queueIndices.size() : 0_zu),
		.pQueueFamilyIndices = ctx.uniquePresentQueue()? queueIndices.data() : nullptr,
		.preTransform = ctx.surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surfacePresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = old,
	};
	VK(vkCreateSwapchainKHR(ctx.device, &swapchainCI, nullptr, &swapchain));
	vk::setDebugName(ctx.device, swapchain, "Swapchain::swapchain");

	// Retrieve swapchain images
	u32 swapchainImageCount;
	vkGetSwapchainImagesKHR(ctx.device, swapchain, &swapchainImageCount, nullptr);
	auto swapchainImagesRaw = std::vector<VkImage>{swapchainImageCount};
	vkGetSwapchainImagesKHR(ctx.device, swapchain, &swapchainImageCount, swapchainImagesRaw.data());
	color.resize(swapchainImagesRaw.size());
	for (auto[raw, image]: zip_view{swapchainImagesRaw, color}) {
		image = vk::Image{
			.image = raw,
			.format = surfaceFormat.format,
			.aspect = VK_IMAGE_ASPECT_COLOR_BIT,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.size = extent,
		};
		image.view = vk::createImageView(ctx.device, image);
		vk::setDebugName(ctx.device, image, fmt::format("Swapchain::color[{}]", &image - &color[0]));
	}

	L.info("Created a Vulkan swapchain at {}x{} with {} images",
		extent.width, extent.height, color.size());
}

void Swapchain::cleanup(Context& ctx) {
	for (auto& image: color)
		vk::destroyImage(ctx.device, ctx.allocator, image);
	vkDestroySwapchainKHR(ctx.device, swapchain, nullptr);
}

}
