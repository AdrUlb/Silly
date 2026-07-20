#pragma once
#include <iterator>

namespace Silly
{
	template<typename T> struct PointerIterator
	{
		using iterator_category = std::contiguous_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;

		explicit constexpr PointerIterator() : _ptr(nullptr) {}

		explicit constexpr PointerIterator(const pointer ptr) : _ptr(ptr) {}

		constexpr reference operator*() const
		{
			return *_ptr;
		}

		constexpr pointer operator->() const
		{
			return _ptr;
		}

		constexpr PointerIterator& operator++()
		{
			_ptr++;
			return *this;
		}

		constexpr PointerIterator operator++(int)
		{
			const auto tmp = *this;
			operator++();
			return tmp;
		}

		constexpr PointerIterator& operator--()
		{
			_ptr--;
			return *this;
		}

		constexpr PointerIterator operator--(int)
		{
			const auto tmp = *this;
			operator--();
			return tmp;
		}

		constexpr PointerIterator& operator +=(const difference_type offset)
		{
			_ptr += offset;
			return *this;
		}

		constexpr PointerIterator& operator -=(const difference_type offset)
		{
			_ptr -= offset;
			return *this;
		}

		[[nodiscard]] constexpr friend PointerIterator operator+(PointerIterator it, const difference_type offset)
		{
			return PointerIterator { it._ptr + offset };
		}

		[[nodiscard]] constexpr friend PointerIterator operator+(const difference_type offset, PointerIterator it)
		{
			return PointerIterator { it._ptr + offset };
		}

		[[nodiscard]] constexpr friend PointerIterator operator-(PointerIterator it, const difference_type offset)
		{
			return PointerIterator { it._ptr - offset };
		}

		[[nodiscard]] constexpr friend difference_type operator-(const PointerIterator leftIt, PointerIterator rightIt)
		{
			return leftIt._ptr - rightIt._ptr;
		}

		[[nodiscard]] constexpr friend bool operator==(const PointerIterator& a, const PointerIterator& b)
		{
			return a._ptr == b._ptr;
		}

		[[nodiscard]] constexpr friend bool operator!=(const PointerIterator& a, const PointerIterator& b)
		{
			return a._ptr != b._ptr;
		}

		[[nodiscard]] constexpr friend auto operator<=>(const PointerIterator& a, const PointerIterator& b)
		{
			return a._ptr <=> b._ptr;
		}

		[[nodiscard]] constexpr reference operator[](const difference_type offset) const
		{
			return _ptr[offset];
		}

	private:
		pointer _ptr;
	};

	static_assert(std::contiguous_iterator<PointerIterator<int>>);
} // namespace Silly
