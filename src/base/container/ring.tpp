#pragma once

#include <stdexcept>
#include <algorithm>
#include <utility>
#include <cassert>
#include <memory>

namespace minote::base {

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(size_type _num) {
	
	for (size_type i = 0; i < _num; ++i)
		emplace_back();
	
}

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(size_type _num, value_type const& _val) {
	
	for (size_type i = 0; i < _num; ++i)
		emplace_back(_val);
	
}

template<typename T, std::size_t Capacity>
template<typename InputIt>
constexpr ring<T, Capacity>::ring(InputIt _first, InputIt _last) {
	
	std::for_each(_first, _last, [this](auto const& val) { emplace_back(val); });
	
}

template<typename T, std::size_t Capacity>
constexpr ring<T, Capacity>::ring(std::initializer_list<value_type> _il) {
	
	std::for_each(_il.begin(), _il.end(), [this](auto const& val) { emplace_back(val); });
	
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::swap(ring& _other) {
	
	auto[shorter, longer] = [&]() -> std::pair<ring&, ring&> {
		
		if (size() > _other.size())
			return {_other, *this};
		else
			return {*this, _other};
		
	}();
	
	std::swap_ranges(shorter.begin(), shorter.end(), longer.begin());
	
	auto const shorterSize = shorter.m_size();
	auto begin = longer.begin();
	auto end = longer.end();
	for (auto i = begin + shorterSize; i != end; ++i) {
		
		shorter.emplace_back(std::move(*i));
		std::destroy_at(i);
		
	}
	
	longer.length = shorterSize;
	
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::at(size_type _i) -> reference {
	
	assert(_i < length);
	size_type index = (offset + _i) % capacity();
	return *std::launder(reinterpret_cast<value_type*>(&buffer[index]));
	
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::at(size_type _i) const -> const_reference {
	
	assert(_i < length);
	size_type index = (offset + _i) % capacity();
	return *std::launder(reinterpret_cast<value_type const*>(&buffer[index]));
	
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::push_back(const_reference _value) {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	length += 1;
	std::construct_at(&back(), _value);
	
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::push_back(value_type&& _value) {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	length += 1;
	std::construct_at(&back(), std::move(_value));
	
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::push_front(const_reference _value) {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), _value);
	
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::push_front(value_type&& _value) {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), std::move(_value));
	
}

template<typename T, size_t Capacity>
template<typename... Args>
constexpr auto ring<T, Capacity>::emplace_back(Args&&... _args) -> reference {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	length += 1;
	std::construct_at(&back(), std::forward<Args>(_args)...);
	return back();
	
}

template<typename T, size_t Capacity>
template<typename... Args>
constexpr auto ring<T, Capacity>::emplace_front(Args&&... _args) -> reference {
	
	if (length == capacity())
		throw std::out_of_range("ring is full");
	
	offset = offset? offset - 1 : capacity() - 1;
	length += 1;
	std::construct_at(&front(), std::forward<Args>(_args)...);
	return front();
	
}

template<typename T, size_t Capacity>
constexpr void ring<T, Capacity>::pop_front() {
	
	assert(length > 0);
	
	std::destroy_at(&front());
	offset += 1;
	offset %= capacity();
	length -= 1;
	
}

template<typename T, std::size_t Capacity>
constexpr void ring<T, Capacity>::pop_back() {
	
	assert(length > 0);
	
	std::destroy_at(&back());
	length -= 1;
	
}

template<typename T, std::size_t Capacity>
constexpr auto ring<T, Capacity>::operator=(std::initializer_list<value_type> _il) -> ring& {
	
	clear();
	std::for_each(_il.begin(), _il.end(), [this](auto const& val) { emplace_back(val); });
	
}

}
