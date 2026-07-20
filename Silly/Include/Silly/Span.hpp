#pragma once
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <type_traits>

#include "Macros.hpp"
#include "PointerIterator.hpp"
#include "Hashing/HashAlgorithm.hpp"

namespace Silly
{
	template<typename T> class Span
	{
		template<typename> friend class Span;

	public:
		using iterator = PointerIterator<T>;

		constexpr Span() : _ptr(nullptr), _length(0) {}

		constexpr Span(T* ptr, const size_t length) : _ptr(ptr), _length(length) {}

		// Copy constructor from span of non-const to const of the same fundamental type
		template<typename U>
		constexpr Span(const Span<U>& other)
			requires(!std::is_same_v<T, U> && std::is_same_v<std::remove_const_t<T>, U>) : _ptr(other._ptr), _length(other._length) {}

		template<size_t length>
		constexpr Span(T (&arr)[length]) : _ptr(arr), _length(length) {}

		constexpr Span(const Span& other) = default;
		constexpr Span& operator=(const Span& other) = default;

		// Copy assignment from span of non-const to const of the same fundamental type
		template<typename U>
		constexpr Span& operator=(const Span<U>& other)
			requires(!std::is_same_v<T, U> && std::is_same_v<std::remove_const_t<T>, U>)
		{
			_ptr = other._ptr;
			_length = other._length;
			return *this;
		}

		[[nodiscard]] constexpr FORCE_INLINE iterator begin() const
		{
			return iterator(_ptr);
		}

		[[nodiscard]] constexpr FORCE_INLINE iterator end() const
		{
			return iterator(_ptr + _length);
		}

		[[nodiscard]] constexpr FORCE_INLINE T* AsPointer() const
		{
			return _ptr;
		}

		[[nodiscard]] constexpr FORCE_INLINE auto AsBytes() const
		{
			using ByteType = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;
			return Span<ByteType> { reinterpret_cast<ByteType*>(_ptr), _length * sizeof(T) };
		}

		[[nodiscard]] constexpr FORCE_INLINE size_t GetLength() const
		{
			return _length;
		}

		[[nodiscard]] constexpr FORCE_INLINE bool IsEmpty() const
		{
			return _length == 0;
		}

		FORCE_INLINE constexpr void Reverse()
			requires(!std::is_const_v<T>)
		{
			for (size_t i = 0; i < _length / 2; i++)
				std::swap(_ptr[i], _ptr[_length - 1 - i]);
		}

		template<typename U>
		constexpr void ReverseTo(Span<U> destination) const
			requires(std::is_convertible_v<T, U> && !std::is_const_v<U>)
		{
			VERIFY(GetLength() <= destination.GetLength());

			const auto s = AsPointer();
			const auto d = destination.AsPointer();
			const auto l = GetLength();

			if constexpr (std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>)
			{
				if (std::less<const void*> { }(d, s + l) && std::less<const void*> { }(s, d + l))
				{
					CopyTo(destination);
					destination.Reverse();
					return;
				}
			}

			for (size_t i = 0; i < l; i++)
				d[i] = s[l - 1 - i];
		}

		[[nodiscard]] constexpr Span Slice(const size_t offset, const size_t length) const
		{
			if (_length == 0)
				return Span { };

			VERIFY(offset <= _length);
			VERIFY(offset + length <= _length);

			return Span { _ptr + offset, length };
		}

		[[nodiscard]] constexpr Span Slice(const size_t offset) const
		{
			VERIFY(offset <= _length);
			return Slice(offset, _length - offset);
		}

		template<typename U>
		constexpr void CopyTo(Span<U> destination) const
			requires(std::is_convertible_v<T, U> && !std::is_const_v<U>)
		{
			VERIFY(GetLength() <= destination.GetLength());

			const auto s = AsPointer();
			const auto d = destination.AsPointer();
			const auto l = GetLength();

			if (std::less_equal<const void*> { }(d, s) || std::greater_equal<const void*> { }(d, s + l))
			{
				std::copy(begin(), end(), destination.begin());
				return;
			}

			std::copy_backward(begin(), end(), destination.begin() + GetLength());
		}

		[[nodiscard]] constexpr T& operator[](size_t index) const
		{
			VERIFY(index < GetLength());
			return AsPointer()[index];
		}

		template<Hashing::HashAlgorithm HashAlgo>
		constexpr friend void ProcessHash(HashAlgo& algorithm, const Span& span)
		{
			algorithm.ProcessBytes(span.AsBytes());
		}

	protected:
		T* _ptr;
		size_t _length;
	};
} // namespace Silly
