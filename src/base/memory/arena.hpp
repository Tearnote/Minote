#pragma once

#include <string_view>
#include <string>
#include "base/types.hpp"

namespace minote::base {

struct Arena {
	
	Arena(std::string_view name, usize capacity);
	~Arena();
	
	auto allocate(usize bytes, usize align) -> void*;
	
	void reset();
	
	Arena(Arena const&) = delete;
	auto operator=(Arena const&) -> Arena& = delete;
	Arena(Arena&&);
	auto operator=(Arena&&) -> Arena&;
	
// protected:
	
	std::string name;
	void* mem;
	usize capacity;
	usize used;
	
};

}
