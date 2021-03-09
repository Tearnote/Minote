#pragma once

#include <initializer_list>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <memory>
#include <limits>
#include <array>

namespace minote::base {

template<typename T, std::size_t Capacity>
struct ring {

	template<typename U>
	struct iter;

	using value_type = T;
	using reference = value_type&;
	using const_reference = value_type const&;
	using iterator = iter<value_type>;
	using const_iterator = iter<value_type const>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = std::ptrdiff_t;
	using size_type = std::size_t;

	constexpr ring() noexcept = default;
	constexpr ring(ring const&) noexcept = default;
	constexpr explicit ring(size_type num);
	constexpr ring(size_type num, value_type const& val);
	template<typename InputIt>
	constexpr ring(InputIt first, InputIt last);
	constexpr ring(std::initializer_list<value_type>);
	constexpr ~ring() { clear(); };
	constexpr void swap(ring& other);

	constexpr auto size() const { return length; }
	constexpr auto empty() const { return length == 0; }
	constexpr auto isEmpty() const { return empty(); } // Nonstandard
	constexpr auto isFull() const { return length == Capacity; } // Nonstandard
	constexpr auto capacity() const { return Capacity; }
	constexpr auto max_size() const { return std::numeric_limits<size_type>::max(); }

	constexpr auto at(size_type i) -> reference;
	constexpr auto at(size_type i) const -> const_reference;
	constexpr auto front() -> reference { return at(0); }
	constexpr auto front() const -> const_reference { return at(0); }
	constexpr auto back() -> reference { return at(length - 1); }
	constexpr auto back() const -> const_reference { return at(length - 1); }

	constexpr auto begin() { return iterator{*this, 0}; }
	constexpr auto begin() const { return const_iterator{*this, 0}; }
	constexpr auto cbegin() const { return begin(); }
	constexpr auto end() { return iterator{*this, size()}; }
	constexpr auto end() const { return const_iterator{*this, size()}; }
	constexpr auto cend() const { return end(); }
	constexpr auto rbegin() { return reverse_iterator{end()}; }
	constexpr auto rbegin() const { return const_reverse_iterator{end()}; }
	constexpr auto crbegin() const { return rbegin(); }
	constexpr auto rend() { return reverse_iterator{begin()}; }
	constexpr auto rend() const { return const_reverse_iterator{begin()}; }
	constexpr auto crend() const { return rend(); }

	constexpr void push_back(const_reference value);
	constexpr void push_back(value_type&& value);
	constexpr void push_front(const_reference value);
	constexpr void push_front(value_type&& value);
	template<typename... Args>
	constexpr auto emplace_back(Args&&... args) -> reference;
	template<typename... Args>
	constexpr auto emplace_front(Args&&... args) -> reference;
	constexpr void pop_front();
	constexpr void pop_back();
	constexpr void clear() { std::destroy_n(begin(), size()); length = 0; }

	constexpr auto operator=(ring const&) -> ring& = default;
	constexpr auto operator=(std::initializer_list<value_type>) -> ring&;
	constexpr auto operator[](size_type const index) -> reference { return at(index); }
	constexpr auto operator[](size_type const index) const -> const_reference { return at(index); }
//	constexpr auto operator==(ring const& other) const { return std::equal(begin(), end(), other.begin()); }
//	constexpr auto operator!=(ring const&) const -> bool = default;

private:

	std::array<std::aligned_storage_t<sizeof(value_type), alignof(value_type)>, Capacity> buffer;
	size_type offset = 0;
	size_type length = 0;

};

template<typename T, std::size_t Capacity>
template<typename U>
struct ring<T, Capacity>::iter {

	static_assert(std::is_same_v<T, std::remove_cv_t<U>>);

	using parent_type = std::conditional_t<std::is_const_v<U>, ring<T, Capacity> const, ring<T, Capacity>>;

	using iterator_category = std::random_access_iterator_tag;
	using value_type = U;
	using difference_type = typename parent_type::difference_type;
	using pointer = value_type*;
	using reference = value_type&;

	constexpr iter(parent_type& p, size_type i): parent{p}, index{i} {}
	constexpr iter(iter const&) = default;

	constexpr auto operator<=>(iter const& other) const {return index <=> other.index; }
	constexpr auto operator==(iter const& other) const { return index == other.index; }
	constexpr auto operator!=(iter const& other) const -> bool = default;
	constexpr auto operator-(iter const& other) const {
		return static_cast<difference_type>(index) - static_cast<difference_type>(other.index);
	}

	constexpr auto operator*() -> reference { return parent[index]; }
	constexpr auto operator[](difference_type const i) -> reference { return parent[index + i]; }

	constexpr auto operator++() -> iter& { index += 1; return *this; }
	constexpr auto operator++(int) -> iter { index += 1; return iter{parent, index - 1}; }
	constexpr auto operator--() -> iter& { index -= 1; return *this; }
	constexpr auto operator--(int) -> iter { index -= 1; return iter{parent, index + 1}; }
	constexpr auto operator+=(difference_type const i) -> iter& { index += i; return *this; }
	constexpr auto operator-=(difference_type const i) -> iter& { index -= i; return *this; }
	friend constexpr auto operator+(iter self, iter const& other) -> iter { self += other; return self; }
	friend constexpr auto operator-(iter self, iter const& other) -> iter { self -= other; return self; }

private:

	parent_type& parent;
	size_type index;

};

}

#include "ring.tpp"
