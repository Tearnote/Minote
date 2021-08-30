#pragma once

#include <type_traits>
#include <variant>
#include <span>
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "vuk/Image.hpp"
#include "vuk/Name.hpp"
#include "base/containers/hashmap.hpp"
#include "gfx/resources/texture2d.hpp"
#include "gfx/resources/cubemap.hpp"
#include "gfx/resources/buffer.hpp"

namespace minote::gfx {

using namespace base;

// A pool for holding resources. 
struct ResourcePool {
	
	// Create a pool with no PTC bound. Use setPtc() to bind one before making
	// any elements.
	ResourcePool() = default;
	
	// Create an empty pool.
	ResourcePool(vuk::PerThreadContext& ptc): m_ptc(&ptc) {}
	
	// Create a texture, and return a Texture2D adapter to it. If a texture
	// with the given name already exists, the texture will be reused.
	template<typename T>
	requires std::is_same_v<T, Texture2D>
	auto make_texture(vuk::Name, uvec2 size, vuk::Format,
		vuk::ImageUsageFlags, u32 mips = 1) -> Texture2D;
	
	// Create a texture, and return a Cubemap adapter to it. If a texture
	// with the given name already exists, the texture will be reused.
	template<typename T>
	requires std::is_same_v<T, Cubemap>
	auto make_texture(vuk::Name, u32 size, vuk::Format, vuk::ImageUsageFlags) -> Cubemap;
	
	// Create an empty buffer, and return a typed Buffer view to it. If a
	// buffer with the given name already exists, the buffer will be reused.
	template<typename T>
	auto make_buffer(vuk::Name, vuk::BufferUsageFlags, usize elements = 1,
		vuk::MemoryUsage = vuk::MemoryUsage::eGPUonly) -> Buffer<T>;
	
	// Create a buffer with specified data. If a buffer with the given name
	// already exists, the buffer will be reused. If memory usage is GPU-only,
	// a transfer will be queued but not waited for.
	template<typename T>
	auto make_buffer(vuk::Name, vuk::BufferUsageFlags, std::span<T const> data,
		vuk::MemoryUsage = vuk::MemoryUsage::eCPUtoGPU) -> Buffer<T>;
	
	// Bind a new PerFrameContext. For pools used on multiple frames.
	void setPtc(vuk::PerThreadContext& ptc) { m_ptc = &ptc; }
	
	// Return the current PerFrameContext.
	auto ptc() -> vuk::PerThreadContext& { return *m_ptc; }
	
	// Enqueue destruction of all resources in the pool.
	void reset() { m_resources.clear(); }
	
private:
	
	vuk::PerThreadContext* m_ptc;
	
	using Resource = std::variant<vuk::Unique<vuk::Buffer>, vuk::Texture>;
	hashmap<vuk::Name, Resource> m_resources;
	
};

}

#include "gfx/resources/resourcepool.tpp"
