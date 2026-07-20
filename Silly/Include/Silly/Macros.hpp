#pragma once

#define FORCE_INLINE __attribute__((always_inline)) inline
#define NEVER_INLINE __attribute__((noinline))
#define RARELY_USED __attribute__((cold))
#define USED __attribute__((used))
#define WEAK __attribute__((weak))

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define STRINGIFY(...) #__VA_ARGS__

#define VERIFY(...) if (!(__VA_ARGS__)) [[unlikely]] Silly::Extern::VerifyFailed(STRINGIFY(__VA_ARGS__))
#define VERIFY_NOT_REACHED(...) ({ [[unlikely]] Silly::Extern::VerifyFailed(STRINGIFY(__VA_ARGS__)); })
#define IGNORE(...) (void)(__VA_ARGS__)

#include "Extern.hpp"
