/**
 * Template implementation of varray.hpp
 * @file
 */

#pragma once

#include "varray.hpp"

#include <cstring>
#include "base/util.hpp"

namespace minote {

template<typename T, std::size_t N>
auto varray<T, N>::produce() -> Element*
{
	if (size == Capacity)
		return nullptr;

	size += 1;
	return &buffer[size - 1];
}

template<typename T, std::size_t N>
void varray<T, N>::remove(std::size_t index)
{
	ASSERT(index < size);
	if (index < size - 1) {
		std::memmove(buffer + index * sizeof(Element),
			buffer + (index + 1) * sizeof(Element),
			(size - index - 1) * sizeof(Element));
	}
	size -= 1;
}

template<typename T, std::size_t N>
void varray<T, N>::removeSwap(std::size_t index)
{
	ASSERT(index < size);
	if (index < size - 1)
		buffer[index] = buffer[size - 1];
	size -= 1;
}

template<typename T, std::size_t N>
void varray<T, N>::clear()
{
	size = 0;
}

template<typename T, std::size_t N>
auto varray<T, N>::operator[](std::size_t index) -> Element&
{
	ASSERT(index < size);
	return buffer[index];
}

template<typename T, std::size_t N>
constexpr auto varray<T, N>::operator[](std::size_t index) const -> const Element&
{
	ASSERT(index < size);
	return buffer[index];
}

template<typename T, std::size_t N>
auto varray<T, N>::data() -> Element*
{
	return buffer.data();
}

template<typename T, std::size_t N>
constexpr auto varray<T, N>::data() const -> const Element*
{
	return buffer.data();
}

}
