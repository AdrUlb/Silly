#pragma once
#include <memory>
#include <utility>

#include "Formatter.hpp"
#include "String.hpp"
#include "Memory/Allocator.hpp"

namespace Silly
{
	template<typename T>
	class Box
	{
	public:
		Box() : _allocator(nullptr), _ptr(nullptr) {}
		Box(std::nullptr_t) : _allocator(nullptr), _ptr(nullptr) {}

		~Box()
		{
			Clear();
		}

		Box(const Box&) = delete;
		Box& operator=(const Box&) = delete;

		Box(Box&& other) noexcept : _allocator(other._allocator), _ptr(std::exchange(other._ptr, nullptr)) {}

		operator T&()
		{
			VERIFY(_ptr);
			return *_ptr;
		}

		Box& operator=(Box&& other) noexcept
		{
			if (&other == this)
				return *this;

			Clear();
			_ptr = std::exchange(other._ptr, nullptr);
			_allocator = other._allocator;

			return *this;
		}

		T* operator->() const
		{
			return _ptr;
		}

		T& operator*() const
		{
			return *_ptr;
		}

		operator bool() const
		{
			return _ptr;
		}

		template<typename... Args>
		[[nodiscard]] static Result<Box, Error> New(Memory::Allocator& allocator, Args&&... args)
		{
			const auto ptr = TRY(allocator.Allocate<T>());
#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(ptr, std::forward<Args>(args)...);
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				allocator.Deallocate(ptr);
				return Err(Error("An exception occured while constructing the object."));
			}
#endif
			return Ok(Box { allocator, ptr });
		}

		[[nodiscard]] friend Result<void, Error> AppendToString(String& output, const Box& value, const StringView format) noexcept
		{
			if (!value._ptr)
				return Formatter::FormatValue(output, "null", format);

			return Formatter::FormatValue(output, *value._ptr, format);
		}

	private:
		Box(Memory::Allocator& allocator, T* ptr) : _allocator(&allocator), _ptr(ptr) {}

		void Clear()
		{
			if (!_ptr)
				return;

			_allocator->Delete(_ptr);
			_ptr = nullptr;
		}

		Memory::Allocator* _allocator;
		T* _ptr;
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
