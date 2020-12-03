#pragma once

#include <stdexcept>
#include <utility>
#include "base/util.hpp"
#include "queue.hpp"

namespace minote {

template<typename T, std::size_t Capacity>
struct ring_buffer<T, Capacity>::iterator {

	using parent_type = ring_buffer<T, Capacity>;

	using iterator_category = std::random_access_iterator_tag;
	using value_type = parent_type::value_type;
	using difference_type = parent_type::difference_type;
	using pointer = parent_type::value_type*;
	using reference = parent_type::reference;

};

template<typename T, std::size_t Capacity>
auto ring_buffer<T, Capacity>::at(size_type const i) -> reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return std::launder(*reinterpret_cast<value_type*>(&buffer[index]));
}

template<typename T, std::size_t Capacity>
auto ring_buffer<T, Capacity>::at(size_type const i) const -> const_reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return std::launder(*reinterpret_cast<value_type const*>(&buffer[index]));
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::push_back(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	length += 1;
	new(&back()) value_type{value};
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::push_back(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	length += 1;
	new(&back()) value_type{std::move(value)};
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::push_front(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	new(&front()) value_type{value};
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::push_front(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	new(&front()) value_type{std::move(value)};
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::pop_front() {
	front().~value_type();
	offset += 1;
	offset %= capacity();
	length -= 1;
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::pop_back() {
	back().~value_type();
	length -= 1;
}

}
