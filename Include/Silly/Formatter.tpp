#pragma once
#include "Formatter.hpp"

#include "String.hpp"

namespace Silly
{
	template<typename... Args>
	[[nodiscard]] Result<void, Error> Formatter::FormatString(String& output, const StringView format, Args&&... args)
	{
		const auto len = output.GetLength();

		const auto result = FormatStringImpl(output, format, std::forward<Args>(args)...);
		if (!result)
			IGNORE(output.SetLength(len));

		return result;
	}

	template<typename T, typename... Args>
	[[nodiscard]] Result<void, Error> Formatter::FormatStringImpl(String& output, const StringView format, T&& arg, Args&&... args)
	{
		const auto startIndexResult = format.Find('{');
		if (!startIndexResult)
			return output.Append(format);
		const auto startIndex = startIndexResult.UnwrapOk();

		const auto endIndexResult = format.Find('}', startIndex + 1);
		if (!endIndexResult)
			return output.Append(format);
		const auto endIndex = endIndexResult.UnwrapOk();

		TRY(output.Append(format.Slice(0, startIndex)));

		const auto remainder = format.Slice(endIndex + 1);
		const auto fmt = format.Slice(startIndex + 1, endIndex - startIndex - 1);
		TRY(FormatValue(output, arg, fmt));

		return FormatStringImpl(output, remainder, std::forward<Args>(args)...);
	}

	template<std::integral T>
	[[nodiscard]] Result<void, Error> Formatter::FormatValue(String& output, T value, const StringView format)
	{
		const auto len = output.GetLength();

		IntegerFormat fmt;
		ParseIntegerFormat(format, fmt);

		const auto result = std::is_signed_v<T>
		                    ? FormatSignedIntegerImpl(output, static_cast<intmax_t>(value), static_cast<std::make_unsigned_t<T>>(value), fmt)
		                    : FormatUnsignedIntegerImpl(output, static_cast<uintmax_t>(value), fmt);

		if (!result)
			IGNORE(output.SetLength(len));

		return result;
	}

	template<std::convertible_to<StringView> T>
	[[nodiscard]] Result<void, Error> Formatter::FormatValue(String& output, const T& value, StringView format) requires(!Formattable<T>)
	{
		const auto len = output.GetLength();

		const auto view = static_cast<StringView>(value);

		auto justifyLeft = false;
		if (format.GetLength() > 0 && format[0] == '-')
		{
			justifyLeft = true;
			format = format.Slice(1);
		}

		size_t minWidth = 0;
		TryParseFormatNumber(format, minWidth);

		const auto result = FormatValueImplStringView(output, view, justifyLeft, minWidth);

		if (!result)
			IGNORE(output.SetLength(len));

		return result;
	}
}

#include "Formatter.tpp"
