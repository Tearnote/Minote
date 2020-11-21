#pragma once

#include <cstring>
#include "base/util.hpp"

namespace minote {

template<typename T, size_t N>
auto varray<T, N>::produce() -> Element*
{
	if (_size == Capacity) return nullptr;

	_size += 1;
	auto* const result = &buffer[_size - 1];
	std::memset(result, 0, sizeof(Element));
	return result;
}

template<typename T, size_t N>
void varray<T, N>::remove(size_t const index)
{
	ASSERT(index < _size);
	if (index < _size - 1) {
		std::memmove(buffer + index * sizeof(Element),
			buffer + (index + 1) * sizeof(Element),
			(_size - index - 1) * sizeof(Element));
	}
	_size -= 1;
}

template<typename T, size_t N>
void varray<T, N>::removeSwap(size_t const index)
{
	ASSERT(index < _size);
	if (index < _size - 1)
		buffer[index] = buffer[_size - 1];
	_size -= 1;
}

template<typename T, size_t N>
void varray<T, N>::clear()
{
	_size = 0;
}

template<typename T, size_t N>
auto varray<T, N>::operator[](size_t const index) -> Element&
{
	ASSERT(index < _size);
	return buffer[index];
}

template<typename T, size_t N>
auto varray<T, N>::operator[](size_t const index) const -> Element const&
{
	ASSERT(index < _size);
	return buffer[index];
}

template<typename T, size_t N>
auto varray<T, N>::data() -> Element*
{
	return buffer.data();
}

template<typename T, size_t N>
auto varray<T, N>::data() const -> Element const*
{
	return buffer.data();
}

}
