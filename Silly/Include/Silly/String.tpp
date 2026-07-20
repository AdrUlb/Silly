#pragma once
#include "String.hpp"

#include "Formatter.hpp"

namespace Silly
{
	template<typename... Args>
	[[nodiscard]] FORCE_INLINE Result<String, Error> String::Format(Memory::Allocator& allocator, StringView format, Args&&... args)
	{
		String str(allocator);
		TRY(Formatter::FormatString(str, format, std::forward<Args>(args)...));
		return Ok(std::move(str));
	}
}
