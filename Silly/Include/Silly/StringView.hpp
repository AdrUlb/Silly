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

		[[nodiscard]] static Result<void, Error> AppendToStringImpl(String& str, const StringView& view);
	};
} // namespace Silly
