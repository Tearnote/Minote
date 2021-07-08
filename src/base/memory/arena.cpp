#include "base/memory/arena.hpp"

#include <cstdlib>
#include <utility>
#include "base/error.hpp"
#include "base/util.hpp"
#include "base/log.hpp"

namespace minote::base {

Arena::Arena(string_view _name, usize _capacity):
	m_name(_name), m_capacity(_capacity) {
	
	m_used = 0;
	m_mem = std::malloc(m_capacity);
	if (!m_mem)
		throw runtime_error_fmt(
			"Failed to allocate {} bytes for allocator {}",
			m_capacity, m_name);
	
	L_DEBUG("Created allocator {} with capacity of {} bytes", m_name, m_capacity);
	
}

Arena::~Arena() {
	
	std::free(m_mem);
	
}

auto Arena::allocate(usize _bytes, usize _align) -> void* {
	
	auto offset = align(m_used, _align);
	auto ptr = (void*)((char*)(m_mem) + offset);
	
	m_used = offset + _bytes;
	if (m_used > m_capacity)
		throw runtime_error_fmt(
			"Allocator {} over capacity: current usage is {} bytes out of {}",
			m_name, m_used, m_capacity);
	
	if (f64(m_used) / f64(m_capacity) >= 0.95)
		L_DEBUG("Allocator {} at 95% usage", m_name);
	else if (f64(m_used) / f64(m_capacity) >= 0.90)
		L_DEBUG("Allocator {} at 90% usage", m_name);
	else if (f64(m_used) / f64(m_capacity) >= 0.80)
		L_DEBUG("Allocator {} at 80% usage", m_name);
	
	return ptr;
	
}

void Arena::reset() {
	
	m_used = 0;
	
}

Arena::Arena(Arena&& _other) {
	
	*this = std::move(_other);
	
}

auto Arena::operator=(Arena&& _other) -> Arena& {
	
	m_name = std::move(_other.m_name);
	m_mem = _other.m_mem;
	m_capacity = _other.m_capacity;
	m_used = _other.m_used;
	
	_other.m_mem = nullptr; // std::free() is a no-op with nullptr
	_other.m_used = 0;
	
	return *this;
	
}

}
