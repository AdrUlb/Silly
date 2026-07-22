#pragma once
#include <cstddef>

#include "StringView.hpp"

#define SILLY_MAIN(func)	\
	int Main(const Silly::StringView& command, const Silly::Span<const Silly::StringView>& args); \
	int main(int argc, char* argv[]) \
	{ \
		using namespace Silly; \
		VERIFY(argc > 0); /* The first argument must be the command used to start the program */ \
		StringView args[argc - 1]; \
		for (int i = 1; i < argc; i++) \
			args[i - 1] = argv[i]; \
		\
		const StringView command = argv[0]; \
		return func(command, Span { args, static_cast<size_t>(argc) - 1 }); \
	}
