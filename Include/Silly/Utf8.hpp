#pragma once
#include <cstdint>

namespace Silly::Utf8
{
	uint32_t GetNextCodepoint(void*& ptr, void* end);
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
