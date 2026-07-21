#include "Silly/Formatter.hpp"

#include "Silly/String.hpp"

namespace Silly
{
	[[nodiscard]] Result<void, Error> Formatter::FormatStringImpl(String& output, const StringView format)
	{
		return output.Append(format);
	}

	[[nodiscard]] Result<void, Error> Formatter::FormatValue(String& output, void* value, const StringView format)
	{
		const auto len = output.GetLength();

		IntegerFormat fmt;
		// Use different defaults for the format but allow overriding in format
		fmt.Base = 16;
		fmt.Precision = sizeof(uintptr_t) * 2;
		ParseIntegerFormat(format, fmt);

		const auto result = FormatUnsignedIntegerImpl(output, reinterpret_cast<uintptr_t>(value), fmt);

		if (!result)
			IGNORE(output.SetLength(len));

		return result;
	}

	Result<void, Error> Formatter::FormatValueImplStringView(String& output, const StringView view, const bool justifyLeft, const size_t minWidth)
	{
		const size_t totalWidth = std::max(view.GetLength(), minWidth);
		TRY(output.EnsureCapacity(output.GetLength() + totalWidth));

		if (justifyLeft)
			TRY(output.Append(view));

		if (minWidth > view.GetLength())
		{
			const auto padWidth = minWidth - view.GetLength();
			for (size_t i = 0; i < padWidth; i++)
				TRY(output.Append(' '));
		}

		if (!justifyLeft)
			TRY(output.Append(view));

		return Ok();
	}

	[[nodiscard]] Result<void, Error> Formatter::FormatSignedIntegerImpl(
		String& output,
		const intmax_t value,
		const uintmax_t zeroExtended, // The caller must zero-extend the value (to convert a -1 byte to 0xFF and not 0xFFFFFFFF)
		const IntegerFormat& fmt)
	{
		uintmax_t absValue;

		if (fmt.Base == 10 && value < 0)
		{
			// Reserve space for two additional characters in the string, append the negative sign bit
			TRY(output.EnsureCapacity(output.GetLength() + 2));
			IGNORE(output.Append('-'));

			absValue = static_cast<uintmax_t>(0) - static_cast<uintmax_t>(value); // Using just unary '-' may cause UB here if value is INT_MIN
		}
		else
		{
			// Ensure a -1 *byte* is converted to 0xFF and not 0xFFFFFFFF
			absValue = zeroExtended;
		}

		return FormatUnsignedIntegerImpl(output, absValue, fmt);
	}

	[[nodiscard]] Result<void, Error> Formatter::FormatUnsignedIntegerImpl(String& output, uintmax_t value, const IntegerFormat& format)
	{
		// Figure out the minimum number of characters we absolutely require, attempt to reserve them ahead of time
		const auto precision = std::max<size_t>(format.Precision, 1);
		TRY(output.EnsureCapacity(output.GetLength() + precision));

		// Fast path for 0
		if (value == 0)
		{
			for (size_t i = 0; i < precision; i++)
				IGNORE(output.Append('0')); // Capacity already ensured above

			return Ok();
		}

		char buf[sizeof(uintmax_t) * CHAR_BIT];
		size_t index = sizeof(buf);

		while (value != 0)
		{
			const auto digitValue = value % format.Base;
			const auto digit = digitValue <= 9 ? static_cast<char>(digitValue + '0') : static_cast<char>(digitValue - 10 + (format.Uppercase ? 'A' : 'a'));
			buf[--index] = digit;
			value /= format.Base;
		}

		const auto digits = sizeof(buf) - index;

		if (precision > digits)
		{
			const auto count = precision - digits;
			TRY(output.EnsureCapacity(output.GetLength() + count));
			for (size_t i = 0; i < count; i++)
				IGNORE(output.Append('0'));
		}

		TRY(output.Append({ buf + index, digits }));
		return Ok();
	}
}
