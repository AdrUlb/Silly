#include <cstdint>
#include <cstring>

#include <Silly/Macros.hpp>

USED WEAK size_t strlen(const char* str) noexcept
{
	auto end = str;

	while (*end)
		end++;

	return end - str;
}

USED WEAK int strcmp(const void* str1, const void* str2) noexcept
{
	const auto ptr1 = static_cast<const unsigned char*>(str1);
	const auto ptr2 = static_cast<const unsigned char*>(str2);

	for (size_t i = 0; ptr1[i] || ptr2[i]; i++)
	{
		if (ptr1[i] > ptr2[i])
			return 1;

		if (ptr1[i] < ptr2[i])
			return -1;
	}

	return 0;
}

USED WEAK void* memcpy(void* dest, const void* src, const size_t count) noexcept
{
	if (!count)
		return dest;

	auto d = static_cast<unsigned char*>(dest);
	auto s = static_cast<const unsigned char*>(src);
	for (size_t i = 0; i < count; i++)
		*d++ = *s++;

	return dest;
}

USED WEAK void* memmove(void* dest, const void* src, const size_t count) noexcept
{
	if (!count)
		return dest;

	auto d = static_cast<unsigned char*>(dest);
	auto s = static_cast<const unsigned char*>(src);

	if (reinterpret_cast<uintptr_t>(d) <= reinterpret_cast<uintptr_t>(s) || reinterpret_cast<uintptr_t>(d) >= reinterpret_cast<uintptr_t>(s) + count)
		return memcpy(dest, src, count);

	d += count - 1;
	s += count - 1;
	for (size_t i = 0; i < count; i++)
		*d-- = *s--;

	return dest;
}

USED WEAK void* memset(void* str, const int c, const size_t count) noexcept
{
	if (!count)
		return str;

	const auto value = static_cast<unsigned char>(c);
	auto ptr = static_cast<unsigned char*>(str);

	for (size_t i = 0; i < count; i++)
		*ptr++ = value;

	return str;
}

USED WEAK int memcmp(const void* str1, const void* str2, const size_t count) noexcept
{
	if (!count)
		return 0;

	const auto ptr1 = static_cast<const unsigned char*>(str1);
	const auto ptr2 = static_cast<const unsigned char*>(str2);
	for (size_t i = 0; i < count; i++)
	{
		if (ptr1[i] < ptr2[i])
			return -1;

		if (ptr1[i] > ptr2[i])
			return 1;
	}

	return 0;
}

USED WEAK void* memchr(void* str, const int c, const size_t count) noexcept
{
	if (!count)
		return nullptr;

	const auto value = static_cast<unsigned char>(c);
	auto ptr = static_cast<const unsigned char*>(str);

	for (size_t i = 0; i < count; i++)
	{
		if (*ptr == value)
			return const_cast<void*>(static_cast<const void*>(ptr));

		ptr++;
	}

	return nullptr;
}

USED WEAK void* memrchr(void* str, const int c, const size_t count) noexcept
{
	if (!count)
		return nullptr;

	const auto value = static_cast<unsigned char>(c);
	auto ptr = static_cast<const unsigned char*>(str) + count - 1;

	for (size_t i = 0; i < count; i++)
	{
		if (*ptr == value)
			return const_cast<void*>(static_cast<const void*>(ptr));

		ptr--;
	}

	return nullptr;
}
