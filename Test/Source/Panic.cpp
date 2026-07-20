#include <print>

#include "Silly/Extern.hpp"
#include "Silly/Macros.hpp"

NEVER_INLINE RARELY_USED void Silly::Extern::VerifyFailed(const char* condition, const std::source_location location)
{
	std::print(stderr,
	           "Fatal Error (Verify Failed): {}\n"
	           "  in {}:{}:{}\n"
	           "  function: {}\n",
	           condition,
	           location.file_name(), location.line(), location.column(),
	           location.function_name());

	abort();
}
