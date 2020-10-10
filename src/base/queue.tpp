/**
 * Template implementation of queue.hpp
 * @file
 */

#pragma once

#include "base/queue.hpp"

namespace minote {

template<typename T, std::size_t N>
auto queue<T, N>::enqueue(const Element& e) -> bool
{
	if (isFull())
		return false;

	buffer[head] = e;
	head = (head + 1) % Capacity;
	return true;
}

template<typename T, std::size_t N>
auto queue<T, N>::dequeue() -> Element*
{
	if (isEmpty())
		return nullptr;

	const auto prevTail = tail;
	tail = (tail + 1) % Capacity;
	return &buffer[prevTail];
}

template<typename T, std::size_t N>
auto queue<T, N>::peek() -> Element*
{
	if (isEmpty())
		return nullptr;

	return &buffer[tail];
}

template<typename T, std::size_t N>
auto queue<T, N>::peek() const -> const Element*
{
	if (isEmpty())
		return nullptr;

	return &buffer[tail];
}

template<typename T, std::size_t N>
auto queue<T, N>::isEmpty() const -> bool
{
	return head == tail;
}

template<typename T, std::size_t N>
auto queue<T, N>::isFull() const -> bool
{
	return (head + 1) % Capacity == tail;
}

template<typename T, std::size_t N>
void queue<T, N>::clear()
{
	head = tail;
}

}
