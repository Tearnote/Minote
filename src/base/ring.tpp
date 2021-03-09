#pragma once

#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
#include "base/assert.hpp" // ASSERT() only

namespace minote::base {

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(size_type const num) {
	for (size_type i = 0; i < num; ++i)
		emplace_back();
}

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(size_type const num, value_type const& val) {
	for (size_type i = 0; i < num; ++i)
		emplace_back(val);
}

template<typename T, std::size_t Capacity>
template<typename InputIt>
constexpr ring<T, Capacity>::ring(InputIt first, InputIt last) {
	std::for_each(first, last, [this](auto const& val) { emplace_back(val); });
}

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(std::initializer_list<value_type> il) {
	std::for_each(il.begin(), il.end(), [this](auto const& val) { emplace_back(val); });
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::swap(ring& other) {
	auto[shorter, longer]{[&]() -> std::pair<ring&, ring&> {
		if (size() > other.size())
			return {other, *this};
		else
			return {*this, other};
	}()};

	std::swap_ranges(shorter.begin(), shorter.end(), longer.begin());

	auto const shorterSize = shorter.m_size();
	for (auto i = longer.begin() + shorterSize; i != longer.end(); ++i) {
		shorter.emplace_back(std::move(*i));
		std::destroy_at(i);
	}
	longer.length = shorterSize;
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::at(size_type const i) -> reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return *std::launder(reinterpret_cast<value_type*>(&buffer[index]));
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::at(size_type const i) const -> const_reference {
	ASSERT(i < length);
	size_type const index = (offset + i) % capacity();
	return *std::launder(reinterpret_cast<value_type const*>(&buffer[index]));
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::push_back(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	length += 1;
	std::construct_at(&back(), value);
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::push_back(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	length += 1;
	std::construct_at(&back(), std::move(value));
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::push_front(const_reference value) {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), value);
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::push_front(value_type&& value) {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), std::move(value));
}

template<typename T, size_t Capacity>
template<typename... Args>
constexpr auto ring<T, Capacity>::emplace_back(Args&&... args) -> reference {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	length += 1;
	std::construct_at(&back(), std::forward<Args>(args)...);
	return back();
}

template<typename T, size_t Capacity>
template<typename... Args>
constexpr auto ring<T, Capacity>::emplace_front(Args&&... args) -> reference {
	if (length == capacity())
		throw std::out_of_range{"ring is full"};

	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), std::forward<Args>(args)...);
	return front();
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::pop_front() {
	ASSERT(length > 0);

	std::destroy_at(&front());
	offset += 1;
	offset %= capacity();
	length -= 1;
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::pop_back() {
	ASSERT(length > 0);

	std::destroy_at(&back());
	length -= 1;
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::operator=(std::initializer_list<value_type> il) -> ring& {
	clear();
	std::for_each(il.begin(), il.end(), [this](auto const& val) { emplace_back(val); });
}

}
