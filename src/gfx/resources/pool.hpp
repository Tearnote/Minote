#pragma once

#include <utility>
#include <variant>
#include <span>
#include "vuk/Context.hpp"
#include "vuk/Buffer.hpp"
#include "vuk/Image.hpp"
#include "vuk/Name.hpp"
#include "base/containers/hashmap.hpp"

namespace minote::gfx {

using namespace base;

// A pool for holding resources. 
struct Pool {
	
	// Create a pool with no PTC bound. Use setPtc() to bind one before making
	// any elements.
	Pool() = default;
	
	// Create an empty pool.
	explicit Pool(vuk::PerThreadContext& ptc): m_ptc(&ptc) {}
	
	// Bind a new PerFrameContext. For pools used on multiple frames.
	void setPtc(vuk::PerThreadContext& ptc) { m_ptc = &ptc; }
	
	// Return the current PerFrameContext.
	auto ptc() -> vuk::PerThreadContext& { return *m_ptc; }
	
	// Enqueue destruction of all resources in the pool.
	void reset() { m_resources.clear(); }
	
	// Interface for resources to insert themselves into the pool. Accepted resources are:
	// vuk::Texture
	// vuk::Unique<vuk::Buffer>
	
	// Check if pool contains a resource by a given name.
	[[nodiscard]]
	auto contains(vuk::Name name) const -> bool { return m_resources.contains(name); }
	
	// Return a resource at the given name. No checking for existence or type.
	template<typename T>
	auto get(vuk::Name name) -> T& { return std::get<T>(m_resources.at(name)); }
	
	// Insert a resource at the given name. If the name is taken, nothing happens. Returns
	// a reference to the resource under that name.
	template<typename T>
	auto insert(vuk::Name name, T&& res) -> T& {
		
		return std::get<T>(m_resources.try_emplace(name, std::in_place_type<T>, std::forward<T>(res)).first->second);
		
	}
	
private:
	
	vuk::PerThreadContext* m_ptc;

	using Resource = std::variant<vuk::Unique<vuk::Buffer>, vuk::Texture>;
	hashmap<vuk::Name, Resource> m_resources;
	
};

}
