#pragma once

#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
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

	iterator(parent_type&, size_type);

};

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::swap(ring_buffer& other) {
	auto[shorter, longer]{[&]() -> std::pair<ring_buffer&, ring_buffer&> {
		if (size() > other.size())
			return {other, *this};
		else
			return {*this, other};
	}()};

	std::swap_ranges(shorter.begin(), shorter.end(), longer.begin());

	auto const shorterSize{shorter.size()};
	for (auto i = longer.begin() + shorterSize; i != longer.end(); ++i) {
		shorter.emplace_back(std::move(*i));
		std::destroy_at(i);
	}
	longer.length = shorterSize;
}

template<typename T, std::size_t Capacity>
auto ring_buffer<T, Capacity>::at(size_type const i) -> reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return *std::launder(reinterpret_cast<value_type*>(&buffer[index]));
}

template<typename T, std::size_t Capacity>
auto ring_buffer<T, Capacity>::at(size_type const i) const -> const_reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return *std::launder(reinterpret_cast<value_type const*>(&buffer[index]));
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::push_back(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	length += 1;
	std::construct_at(&back(), value);
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::push_back(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	length += 1;
	std::construct_at(&back(), std::move(value));
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::push_front(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), value);
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::push_front(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), std::move(value));
}

template<typename T, size_t Capacity>
template<typename... Args>
auto ring_buffer<T, Capacity>::emplace_back(Args&&... args) -> reference {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	length += 1;
	std::construct_at(&back(), args...);
}

template<typename T, size_t Capacity>
template<typename... Args>
auto ring_buffer<T, Capacity>::emplace_front(Args&&... args) -> reference {
	if (length == capacity())
		throw std::out_of_range{"ring_buffer is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), args...);
}

template<typename T, size_t Capacity>
void ring_buffer<T, Capacity>::pop_front() {
	std::destroy_at(&front());
	offset += 1;
	offset %= capacity();
	length -= 1;
}

template<typename T, std::size_t Capacity>
void ring_buffer<T, Capacity>::pop_back() {
	std::destroy_at(&back());
	length -= 1;
}

}
