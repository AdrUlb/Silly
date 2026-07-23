#pragma once
#include <cstddef>
#include <span>

#include "../Error.hpp"
#include "../Result.hpp"
#include "../Span.hpp"

namespace Silly::Memory
{
	FORCE_INLINE bool IsValidAlignment(const size_t alignment)
	{
		// Valid alignments must be a power of 2
		return std::has_single_bit(alignment);
	}

	FORCE_INLINE size_t AlignUp(const size_t value, const size_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	class Allocator
	{
	public:
		struct AllocSpec
		{
			size_t Size;
			size_t Alignment;
		};

		virtual ~Allocator() = default;
		[[nodiscard]] virtual Result<void*, Error> AllocateBytes(size_t size, size_t alignment = sizeof(max_align_t)) noexcept = 0;
		virtual void DeallocateBytes(void* ptr, size_t size, size_t alignment = sizeof(max_align_t)) noexcept = 0;

		[[nodiscard]] virtual Result<void*, Error> ReallocateBytes(void* oldPtr, const size_t oldSize, const size_t newSize,
		                                                      const size_t alignment = sizeof(max_align_t)) noexcept
		{
			// When a block of size 0 is requested, deallocate and return success with nullptr
			if (newSize == 0)
			{
				if (oldPtr)
					DeallocateBytes(oldPtr, oldSize, alignment);
				return Ok(nullptr);
			}

			// Attempt to get the new block
			const auto newPtr = TRY(AllocateBytes(newSize, alignment));

			// If there was no old block to copy data from we're done here
			if (!oldPtr)
				return Ok(newPtr);

			const auto oldBytes = Span(static_cast<std::byte*>(oldPtr), oldSize);
			const auto newBytes = Span(static_cast<std::byte*>(newPtr), newSize);

			const auto copySlice = oldBytes.Slice(0, std::min(oldSize, newSize));
			copySlice.CopyTo(newBytes);
			DeallocateBytes(oldPtr, oldSize, alignment);
			return Ok(newPtr);
		}

		[[nodiscard]] Result<void*, Error> Allocate(const AllocSpec spec)
		{
			return AllocateBytes(spec.Size, spec.Alignment);
		}

		void Deallocate(void* ptr, const AllocSpec spec)
		{
			DeallocateBytes(ptr, spec.Size, spec.Alignment);
		}

		template<typename T> [[nodiscard]] Result<T*, Error> Allocate()
		{
			const auto alloc = TRY(AllocateBytes(sizeof(T), alignof(T)));
			return Ok(static_cast<T*>(alloc));
		}

		template<typename T> void Deallocate(T* ptr)
		{
			DeallocateBytes(ptr, sizeof(T), alignof(T));
		}

		template<typename T> void Delete(T* ptr)
		{
			std::destroy_at(ptr);
			Deallocate(ptr);
		}

		template<typename T> [[nodiscard]] Result<Span<T>, Error> AllocateSpan(const size_t count)
		{
			const auto ptr = static_cast<T*>(TRY(AllocateBytes(count * sizeof(T), alignof(T))));
			return Ok(Span<T> { ptr, count });
		}

		template<typename T> void DeallocateSpan(const Span<T> span)
		{
			DeallocateBytes(span.AsPointer(), span.GetLength() * sizeof(T), alignof(T));
		}

		template<typename T>
		[[nodiscard]] Result<Span<T>, Error> ReallocateSpan(const Span<T> span, const size_t newCount)
			requires(std::is_trivially_copyable_v<T>)
		{
			const auto ptr = TRY(ReallocateBytes(span.AsPointer(), span.GetLength() * sizeof(T), newCount * sizeof(T), alignof(T)));
			return Ok(Span<T> { static_cast<T*>(ptr), newCount });
		}
	};
} // namespace Silly

#if SILLY_GLOBAL
using namespace Silly;
#endif
