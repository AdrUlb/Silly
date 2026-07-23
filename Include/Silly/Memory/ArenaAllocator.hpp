#pragma once

#include "Allocator.hpp"

namespace Silly::Memory
{
	template<size_t arenaSize>
	class ArenaAllocator final : public Allocator
	{
	public:
		explicit ArenaAllocator(const Option<Allocator&> fallback = None) : _fallback(fallback) {}

		[[nodiscard]] Result<void*, Error> AllocateBytes(const size_t size, const size_t alignment) noexcept override
		{
			if (!IsValidAlignment(alignment))
				return Err(Error("Invalid alignment requested."));

			const auto arenaAddr = reinterpret_cast<uintptr_t>(_arena);

			const auto alignedOffset = AlignUp(arenaAddr + _offset, alignment) - arenaAddr;

			const auto newOffset = alignedOffset + size;
			if (newOffset > arenaSize)
			{
				if (!_fallback)
					return Err(Error("Arena is out of memory!"));

				return _fallback->AllocateBytes(size, alignment);
			}

			const auto ptr = _arena + alignedOffset;

			_offset = newOffset;
			return Ok(ptr);
		}

		void DeallocateBytes(void* ptr, const size_t size, const size_t alignment) noexcept override
		{
			if (!ptr)
				return;

			const auto bytePtr = static_cast<std::byte*>(ptr);

			if (bytePtr >= _arena && bytePtr < _arena + arenaSize) // If the pointer is within the arena
			{
				// If the allocation was the most recent one, we can actually undo it!
				// (any alignment that was added to the size is still lost)
				if (bytePtr + size == _arena + _offset)
					_offset -= size;

				return;
			}

			// Allocation was not from the arena, must be from the fallback allocator
			if (_fallback)
				_fallback->DeallocateBytes(ptr, size, alignment);
		}

		[[nodiscard]] Result<void*, Error> Reallocate(void* oldPtr, const size_t oldSize, const size_t newSize, const size_t alignment) noexcept
		{
			if (!IsValidAlignment(alignment))
				return Err(Error("Invalid alignment requested."));

			if (!oldPtr)
			{
				if (oldSize != 0)
					return Err(Error("Reallocating from nullptr with old size > 0."));

				return AllocateBytes(newSize, alignment);
			}

			const auto oldBytePtr = static_cast<std::byte*>(oldPtr);

			if (oldBytePtr >= _arena && oldBytePtr < _arena + arenaSize &&
			    oldBytePtr + oldSize == _arena + _offset &&
			    (reinterpret_cast<uintptr_t>(oldPtr) & (alignment - 1)) == 0)
			{
				const auto newOffset = oldBytePtr - _arena + newSize;

				if (newOffset <= arenaSize) // If the resized allocation size still fits within the arena, we're all good!
				{
					_offset = newOffset;
					return Ok(newSize == 0 ? nullptr : oldPtr);
				}
			}

			// Let the base implementation call Allocate/Deallocate which will correctly use the fallback allocator and copy data if necessary
			return ReallocateBytes(oldPtr, oldSize, newSize, alignment);
		}

		[[nodiscard]] size_t GetRemainingCapacity() const
		{
			return arenaSize - _offset;
		}

		Option<Allocator&> _fallback;
		size_t _offset { };
		std::byte _arena[arenaSize] { };
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
