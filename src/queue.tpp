/**
 * Template implementation of queue.hpp
 * @file
 */

#pragma once

#include "queue.hpp"

template<typename T, std::size_t N>
auto queue<T, N>::enqueue(const Element& e) -> bool
{
	if (isFull())
		return false;

	data[head] = e;
	head = (head + 1) % Capacity;
	return true;
}

template<typename T, std::size_t N>
auto queue<T, N>::dequeue() -> Element*
{
	if (isEmpty())
		return nullptr;

	Element* result = &data[tail];
	tail = (tail + 1) % Capacity;
	return result;
}

template<typename T, std::size_t N>
auto queue<T, N>::peek() -> Element*
{
	if (isEmpty())
		return nullptr;

	return &data[tail];
}

template<typename T, std::size_t N>
auto queue<T, N>::isEmpty() -> bool
{
	return head == tail;
}

template<typename T, std::size_t N>
auto queue<T, N>::isFull() -> bool
{
	return (head + 1) % Capacity == tail;
}

template<typename T, std::size_t N>
void queue<T, N>::clear()
{
	head = tail;
}
