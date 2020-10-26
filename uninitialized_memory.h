#pragma once
#include <cstddef>
#include <type_traits>

// RAII wrapper of uninitialized memory.
template <typename T>
class uninitialized_memory
{
public:
	using value_type = T;
	using size_type = std::size_t;

	explicit uninitialized_memory(size_type size) :
		mem(static_cast<T*>(
			::operator new(
				sizeof(T)*size,
				std::align_val_t{alignof(T)}
			)
		)),
		_size(size)
	{}
	~uninitialized_memory()
	{
		::operator delete(
			mem,
			std::align_val_t{alignof(T)}
		);
	}

	uninitialized_memory() = default;
	uninitialized_memory(const uninitialized_memory&) = delete;
	uninitialized_memory(uninitialized_memory&&) noexcept;
	uninitialized_memory& operator=(const uninitialized_memory&) = delete;
	uninitialized_memory& operator=(uninitialized_memory&&) noexcept;
	friend void swap(uninitialized_memory& a, uninitialized_memory& b) noexcept
	{
		using std::swap;
		swap(a.mem, b.mem);
		swap(a._size, b._size);
	}

	T* get() { return mem; }
	const T* get() const { return mem; }
	operator T*() { return mem; }
	operator const T*() const { return mem; }

	size_type size() { return _size; }
	size_type size() const { return _size; }

private:
	T* mem = nullptr;
	size_type _size;
};

template <typename T>
uninitialized_memory<T>::uninitialized_memory(uninitialized_memory<T>&& x) noexcept :
	mem(x.mem),
	_size(x._size)
{
	x.mem = nullptr;
	x._size = 0;
}

template <typename T>
uninitialized_memory<T>& uninitialized_memory<T>::operator=(uninitialized_memory<T>&& x) noexcept
{
	swap(*this, x);
	return *this;
}
