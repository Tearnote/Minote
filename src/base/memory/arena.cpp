#include "base/memory/arena.hpp"

#include <cstdlib>
#include <utility>
#include "base/error.hpp"
#include "base/log.hpp"

namespace minote::base {

Arena::Arena(string_view _name, usize _capacity):
	name(_name), capacity(_capacity) {
	
	used = 0;
	mem = std::malloc(capacity);
	if (!mem)
		throw runtime_error_fmt(
			"Failed to allocate {} bytes for allocator {}",
			capacity, name);
	
	L_DEBUG("Created arena {} with capacity of {} bytes", name, capacity);
	
}

Arena::~Arena() {
	
	std::free(mem);
	
}

auto Arena::allocate(usize _bytes, usize _align) -> void* {
	
	auto offset = used;
	offset += (_align - offset % _align) % _align;
	auto ptr = (void*)((char*)(mem) + offset);
	
	used = offset + _bytes;
	if (used > capacity)
		throw runtime_error_fmt(
			"Arena {} over capacity: current usage is {} bytes out of {}",
			name, used, capacity);
	
	return ptr;
	
}

void Arena::reset() {
	
	used = 0;
	
}

Arena::Arena(Arena&& _other) {
	
	*this = std::move(_other);
	
}

auto Arena::operator=(Arena&& _other) -> Arena& {
	
	name = std::move(_other.name);
	mem = _other.mem;
	capacity = _other.capacity;
	used = _other.used;
	
	_other.mem = nullptr;
	_other.used = 0;
	
	return *this;
	
}

}
