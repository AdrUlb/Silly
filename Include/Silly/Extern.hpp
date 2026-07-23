#pragma once
#include <source_location>
#include "Macros.hpp"

namespace Silly::Extern
{
	NEVER_INLINE RARELY_USED NORETURN extern void VerifyFailed(const char* condition, std::source_location location = std::source_location::current());
}
