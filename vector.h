#pragma once
#include <algorithm>
#include <memory>
#include <type_traits>
#include <cassert>
#include "uninitialized_memory.h"

// Simplified implementation of std::vector.
// Note that:
// - this implementation doesn't use allocators
// - strong exception guarantee ensured on insertions (depends on T's move ctor's guarantee)
template <typename T>
class vector
{
public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	vector() noexcept :
		storage(0),
		_size(0)
	{}

	explicit vector(size_type size) :
		storage(size),
		_size(size)
	{
		std::uninitialized_value_construct_n(storage, size); // value initialization, as in std::vector
	}

	vector(size_type size, const T& elem) :
		storage(size),
		_size(size)
	{
		std::uninitialized_fill_n(storage.get(), size, elem);
	}

	vector(vector&& x) noexcept :
		vector()
	{
		swap(*this, x);
	}

	vector(const vector& x) :
		storage(x._size),
		_size(x._size)
	{
		std::uninitialized_copy_n(x.storage.get(), x._size, storage.get());
	}

	vector(std::initializer_list<T> ilist) :
		storage(ilist.size()),
		_size(ilist.size())
	{
		std::uninitialized_copy(ilist.begin(), ilist.end(), storage.get());
	}

	template <typename InputIt,
		typename = std::enable_if_t< // make sure that InputIt is an iterator to prevent overload resolution ambiguity with (size, elem) ctor
			std::is_convertible_v<
				typename std::iterator_traits<InputIt>::iterator_category,
				std::input_iterator_tag
			>>> // requires std::input_iterator<InputIt> // C++20
	vector(InputIt first, InputIt last) :
		storage(std::distance(first, last)),
		_size(std::distance(first, last))
	{
		std::uninitialized_copy(first, last, storage.get());
	}

	vector& operator=(vector&& x) noexcept
	{
		swap(*this, x);
		return *this;
	}

	vector& operator=(vector x)
	{
		swap(*this, x);
		return *this;
	}

	vector<T>& operator=(std::initializer_list<T> ilist)
	{
		vector<T> newVector(ilist);
		swap(*this, newVector);
		return *this;
	}

	friend void swap(vector& a, vector& b) noexcept
	{
		using std::swap;
		swap(a.storage, b.storage);
		swap(a._size, b._size);
	}

	~vector()
	{
		std::destroy_n(storage.get(), size());
	}

	void assign(size_type count, const T& value)
	{
		vector<T> newVector(count, value);
		swap(*this, newVector);
	}

	template <typename InputIt,
		typename = std::enable_if_t< // make sure that InputIt is an iterator to prevent overload resolution ambiguity with assign(size, elem)
			std::is_convertible_v<
				typename std::iterator_traits<InputIt>::iterator_category,
				std::input_iterator_tag
			>>> // requires std::input_iterator<InputIt> // C++20
	void assign(InputIt first, InputIt last)
	{
		vector<T> newVector(first, last);
		swap(*this, newVector);
	}

	void assign(std::initializer_list<T> ilist)
	{
		*this = ilist;
	}

	size_type capacity() const { return storage.size(); }
	size_type size() const { return _size; }
	bool empty() const { return size() == 0; }
	T* data() { return storage; }
	const T* data() const { return storage; }

	reference operator[](size_type index) { return storage[index]; }
	const_reference operator[](size_type index) const { return storage[index]; }

	reference at(size_type index)
	{
		if (index > size())
			throw std::out_of_range{"out-of-range access detected (vector)"};
		return storage[index];
	}
	const_reference at(size_type index) const
	{
		if (index > size())
			throw std::out_of_range{"out-of-range access detected (vector)"};
		return storage[index];
	}

	reference front() { return storage[0]; }
	reference back() { return storage[size()-1]; }
	const_reference front() const { return front(); }
	const_reference back() const { return back(); }

	// Iterators:

	using iterator = T*;
	using const_iterator = const T*;
	using reverse_iterator = std::reverse_iterator<T*>;
	using const_reverse_iterator = std::reverse_iterator<const T*>;

	iterator begin() { return iterator{storage}; }
	iterator end() { return iterator{&storage[size()]}; }
	const_iterator begin() const { return const_iterator{storage}; }
	const_iterator end() const { return const_iterator{&storage[size()]}; }

	const_iterator cbegin() const { return const_iterator{storage}; }
	const_iterator cend() const { return const_iterator{&storage[size()]}; }
	reverse_iterator rbegin() { return reverse_iterator{&storage[size()]}; }
	reverse_iterator rend() { return reverse_iterator{storage}; }
	const_reverse_iterator crbegin() const { return const_reverse_iterator{&storage[size]}; }
	const_reverse_iterator crend() const { return const_reverse_iterator{storage}; }

	// Modifiers:

	template <typename U>
	void push_back(U&& u);
	template <typename... Args>
	void emplace_back(Args&&... args);

	template <typename U>
	void insert(iterator it, U&& u);
	template <typename... Args>
	void emplace(iterator it, Args&&... args);

	void pop_back();
	void erase(iterator it);

	void resize(size_type newSize);
	void reserve(size_type newCapacity);
	void clear();
	void shrink_to_fit();

private:
	uninitialized_memory<T> storage;
	size_type _size;

	// Forces capacity change (even if newCap < oldCap)
	// Assumes newCap >= size
	void force_capacity(size_type newCapacity);
};

template <typename T>
template <typename U>
void vector<T>::push_back(U&& u)
{
	if (size() >= capacity())
		reserve(size()*2);
	new (&storage[_size++]) T(std::forward<U>(u));
}

template <typename T>
template <typename... Args>
void vector<T>::emplace_back(Args&&... args)
{
	if (size() >= capacity())
		reserve(size()*2);
	new (&storage[_size++]) T(std::forward<Args>(args)...);
}

template <typename T>
template <typename U>
void vector<T>::insert(iterator it, U&& u)
{
	if (size() >= capacity())
		reserve(size()*2);
	// Move everything from 'it' to the right, then construct in place.
	new (&storage[_size++]) T(std::move(back())); // last element (uninitialized memory)
	std::move( // storage in between (pre-last to 'it', inclusive)
		std::make_reverse_iterator(end()-2),
		std::make_reverse_iterator(it),
		std::make_reverse_iterator(end()-1)
		);
	new (&*it) T(std::forward<U>(u)); // inserted element
}

template <typename T>
template <typename... Args>
void vector<T>::emplace(iterator it, Args&&... args)
{
	if (size() >= capacity())
		reserve(size()*2);
	// Move everything from 'it' to the right, then construct in place.
	new (&storage[_size++]) T(std::move(back())); // last element (uninitialized memory)
	std::move( // storage in between (pre-last to 'it', inclusive)
		std::make_reverse_iterator(end()-2),
		std::make_reverse_iterator(it),
		std::make_reverse_iterator(end()-1)
		);
	new (&*it) T(std::forward<Args>(args)...); // emplaced element
}

template <typename T>
void vector<T>::pop_back()
{
	std::destroy_at(&storage[--_size]);
}

template <typename T>
void vector<T>::erase(vector<T>::iterator it)
{
	// Move assign 1 place to the left
	std::move(it+1, end(), it);
	// Destroy rightmost element (move assign might be implemented as swap)
	std::destroy_at(&storage[--_size]);
}

template <typename T>
void vector<T>::resize(size_type newSize)
{
	if (newSize < size()) {
		// Remove extra elements
		std::destroy(begin()+newSize, end());
	}
	else if (newSize > size()) {
		// Reserve exact amount of memory and value constructs
		reserve(newSize);
		std::uninitialized_value_construct_n(end(), newSize-size()); // value initialization, as in std::vector
	}
	_size = newSize;
}

template <typename T>
void vector<T>::reserve(size_type newCapacity)
{
	if (newCapacity <= capacity())
		return;
	force_capacity(newCapacity);
}

template <typename T>
void vector<T>::clear()
{
	std::destroy(begin(), end());
	_size = 0;
}

template <typename T>
void vector<T>::shrink_to_fit()
{
	if (size() == capacity())
		return;
	force_capacity(size());
}

template <typename T>
void vector<T>::force_capacity(size_type newCapacity)
{
	assert(newCapacity >= size());

	// Note: currently, there's no 'realloc' mechanism - possible improvement
	uninitialized_memory<T> newStorage(newCapacity);

	if constexpr (std::is_nothrow_move_constructible_v<T>) { // nothrow guarantee
		std::uninitialized_move_n(storage.get(), size(), newStorage.get());
	}
	else if constexpr (std::is_copy_constructible_v<T>) { // strong guarantee
		std::uninitialized_copy_n(storage.get(), size(), newStorage.get());
	}
	else { // move ctor could throw -> unspecified result
		try {
			std::uninitialized_move_n(storage.get(), size(), newStorage.get());
		} catch (...) {
			clear();
			throw;
		}
	}

	std::swap(storage, newStorage);
}

// Relational operations:

template <typename T>
bool operator==(const vector<T>& a, const vector<T>& b)
{
	if (a.size() != b.size())
		return false;
	using std::begin;
	using std::end;
	return std::equal(begin(a), end(a), begin(b));
}

template <typename T>
bool operator!=(const vector<T>& a, const vector<T>& b)
{
	return !(a == b);
}

template <typename T>
bool operator<(const vector<T>& a, const vector<T>& b)
{
	using std::begin;
	using std::end;
	return std::lexicographical_compare(begin(a), end(a), begin(b), end(b), std::less<T>());
}

template <typename T>
bool operator>(const vector<T>& a, const vector<T>& b)
{
	return b < a;
}

template <typename T>
bool operator<=(const vector<T>& a, const vector<T>& b)
{
	return !(a > b);
}

template <typename T>
bool operator>=(const vector<T>& a, const vector<T>& b)
{
	return !(a < b);
}
