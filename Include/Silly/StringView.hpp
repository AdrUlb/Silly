#pragma once
#include <compare>

#include "Error.hpp"
#include "Result.hpp"
#include "Span.hpp"

namespace Silly
{
	class String;

	class StringView : public Span<const char>
	{
	public:
		using Span::Span;

		constexpr StringView(const char* cstr, const size_t length)
			: Span(cstr, length) {}

		constexpr StringView(const char* cstr)
			: StringView(cstr, cstr ? std::char_traits<char>::length(cstr) : 0) {}

		template<size_t length>
		constexpr StringView(const char (&str)[length])
			: StringView(str, length - 1) {}

		constexpr StringView(const Span& span)
			: Span(span) {}

		StringView& operator=(const Span& span) noexcept
		{
			Span::operator=(span);
			return *this;
		}

		[[nodiscard]] constexpr bool StartsWith(const StringView start) const noexcept
		{
			return GetLength() >= start.GetLength() && Slice(0, start.GetLength()) == start;
		}

		[[nodiscard]] constexpr bool StartsWith(const char start) const noexcept
		{
			return GetLength() > 0 && _ptr[0] == start;
		}

		[[nodiscard]] constexpr bool EndsWith(const StringView end) const noexcept
		{
			return GetLength() >= end.GetLength() && Slice(GetLength() - end.GetLength()) == end;
		}

		[[nodiscard]] constexpr bool EndsWith(const char end) const noexcept
		{
			return GetLength() > 0 && _ptr[GetLength() - 1] == end;
		}

		template<Hashing::HashAlgorithm HashAlgo>
		constexpr friend void ProcessHash(HashAlgo& algorithm, const StringView& view) noexcept
		{
			algorithm.ProcessBytes(view.AsBytes());
		}

		constexpr friend std::strong_ordering operator<=>(const StringView& a, const StringView& b) noexcept
		{
			return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
		}

		constexpr friend bool operator==(const StringView& a, const StringView& b) noexcept
		{
			return std::is_eq(a <=> b);
		}

		constexpr friend bool operator!=(const StringView& a, const StringView& b) noexcept
		{
			return std::is_neq(a <=> b);
		}

		[[nodiscard]] constexpr Result<size_t, void> Find(const char c, const size_t startIndex = 0) const
		{
			for (size_t i = startIndex; i < _length; i++)
				if (_ptr[i] == c)
					return Ok(i);

			return Err();
		}
	};
} // namespace Silly

#if SILLY_GLOBAL
using namespace Silly;
#endif
