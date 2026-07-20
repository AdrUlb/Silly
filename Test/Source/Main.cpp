#include <print>

#include "Silly/Main.hpp"

#include "Silly/Box.hpp"
#include "Silly/Formatter.hpp"
#include "Silly/String.hpp"
#include "Silly/Memory/ArenaAllocator.hpp"

SILLY_MAIN(Main)

using namespace Silly;

int Main(const StringView& command, const Span<const StringView>& args)
{
	Memory::ArenaAllocator<1024> arena;
	const auto str = String::Format(arena, "{}: 0x{}", "errno location", __errno_location()).UnwrapOk();
	std::println("{}", str.AsCString());
	return 0;
}
