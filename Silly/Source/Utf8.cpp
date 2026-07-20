#include "Silly/Utf8.hpp"

namespace Silly::Utf8
{
	uint32_t GetNextCodepoint(void*& ptr, void* end)
	{
		if (!ptr || !end)
			return 0;

		auto p = static_cast<uint8_t*>(ptr);
		const auto e = static_cast<uint8_t*>(end);

		const auto c = *p++;

		if (c == 0)
			return 0;

		if (c < 0x80)
		{
			ptr = p;
			return c;
		}

		uint32_t codepoint = 0;
		int bytes = 0;

		if ((c & 0xE0) == 0xC0) // 2-byte sequence
		{
			codepoint = c & 0x1F;
			bytes = 1;
		}
		else if ((c & 0xF0) == 0xE0) // 3-byte sequence
		{
			codepoint = c & 0x0F;
			bytes = 2;
		}
		else if ((c & 0xF8) == 0xF0) // 4-byte sequence
		{
			codepoint = c & 0x07;
			bytes = 3;
		}
		else // Invalid
		{
			ptr = p;
			return 0;
		}

		if (p >= e)
			return 0;

		for (int i = 0; i < bytes; i++)
		{
			const auto b = *p;
			if ((b & 0xC0) != 0x80)
			{
				ptr = p;
				return 0;
			}
			codepoint = (codepoint << 6) | (b & 0x3F);
			p++;
		}

		ptr = p;
		return codepoint;
	}
}
