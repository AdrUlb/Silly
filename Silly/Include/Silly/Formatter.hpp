#pragma once
#include <algorithm>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>

#include <Silly/Result.hpp>
#include <Silly/StringView.hpp>

namespace Silly
{
	template<typename T>
	concept Formattable = requires(String& output, const T& value, const StringView format)
	{
		{ AppendToString(output, value, format) } noexcept -> std::same_as<Result<void, Error>>;
	};

	class String;

	struct Formatter
	{
	private:
		struct IntegerFormat
		{
			size_t Base { 10 };
			size_t Precision { 0 };
			bool Uppercase { false };
		};

	public:
		template<typename... Args>
		[[nodiscard]] static Result<void, Error> FormatString(String& output, StringView format, Args&&... args);

		template<typename T, typename... Args>
		[[nodiscard]] static Result<void, Error> FormatStringImpl(String& output, StringView format, T&& arg, Args&&... args);

		[[nodiscard]] static Result<void, Error> FormatStringImpl(String& output, StringView format);

		template<Formattable T>
		[[nodiscard]] static Result<void, Error> FormatValue(String& output, const T& value, const StringView format)
		{
			/*
			 * Depends on the following hidden friend being defined in T:
			 * friend Result<void, Error> AppendToString(String& output, const T& value, StringView format)
			 */
			return AppendToString(output, value, format);
		}

		template<std::integral T>
		[[nodiscard]] static Result<void, Error> FormatValue(String& output, T value, StringView format);

		[[nodiscard]] static Result<void, Error> FormatValue(String& output, void* value, StringView format);

		[[nodiscard]] static Result<void, Error> FormatValue(String& output, const bool value, const StringView)
		{
			return FormatValue(output, value ? "true" : "false", { });
		}

		template<std::convertible_to<StringView> T>
		[[nodiscard]] static Result<void, Error> FormatValue(String& output, const T& value, StringView format) requires(!Formattable<T>);

	private:
		static Result<void, Error> FormatValueImplStringView(String& output, StringView view, bool justifyLeft, size_t minWidth);
		[[nodiscard]] static Result<void, Error> FormatSignedIntegerImpl(String& output, intmax_t value, uintmax_t zeroExtended, const IntegerFormat& fmt);
		[[nodiscard]] static Result<void, Error> FormatUnsignedIntegerImpl(String& output, uintmax_t value, const IntegerFormat& format);

		static constexpr void ParseIntegerFormat(StringView formatString, IntegerFormat& format)
		{
			if (formatString.GetLength() > 0)
			{
				auto baseChar = true;

				switch (formatString[0])
				{
					case 'B':
					case 'b':
						format.Base = 2;
						break;
					case 'o':
					case 'O':
						format.Base = 8;
						break;
					case 'D':
					case 'd':
						format.Base = 10;
						break;
					case 'X':
						format.Base = 16;
						format.Uppercase = true;
						break;
					case 'x':
						format.Base = 16;
						format.Uppercase = false;
						break;
					default:
						baseChar = false;
						break;
				}

				if (baseChar)
					formatString = formatString.Slice(1);
			}

			TryParseFormatNumber(formatString, format.Precision);
		}

		static constexpr size_t TryParseFormatNumber(StringView& format, size_t& value)
		{
			size_t digits = 0;
			size_t ret = 0;

			while (format.GetLength() > 0)
			{
				const auto c = format[0];

				if (c < '0' || c > '9')
					break;

				const auto num = c - '0';

				ret = ret * 10 + num;
				format = format.Slice(1);
				digits++;
			}

			if (digits != 0)
				value = ret;

			return digits;
		}

		explicit Formatter() = default;
	};
}

#include "Formatter.tpp"
