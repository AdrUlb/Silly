#pragma once
#include "Allocator.hpp"
#include "MemoryPool.hpp"

namespace Silly::Memory
{
	class PoolAllocator final : public Allocator
	{
	public:
		PoolAllocator(Allocator& backing, const AllocSpec spec, const size_t blocksPerChunk)
			: _spec(spec), _pool(backing, spec.Size, blocksPerChunk, spec.Alignment) {}

		[[nodiscard]] Result<void*, Error> AllocateBytes(const size_t size, const size_t alignment) noexcept override
		{
			if (!IsValidAlignment(alignment))
				return Err(Error("Invalid alignment requested."));

			if (size > _spec.Size)
				return Err(Error("Size is larger than pool's alloc spec size."));

			if (alignment > _spec.Alignment || _spec.Alignment % alignment != 0)
				return Err(Error("Alignment does not match pool's alloc spec."));

			return _pool.Allocate();
		}

		void DeallocateBytes(void* ptr, const size_t size, const size_t alignment) noexcept override
		{
			IGNORE(size);
			IGNORE(alignment);
			return _pool.Deallocate(ptr);
		}

		[[nodiscard]] Result<void*, Error> ReallocateBytes(void* oldPtr, const size_t oldSize, const size_t newSize, const size_t alignment) noexcept override
		{
			if (!IsValidAlignment(alignment))
				return Err(Error("Invalid alignment requested."));

			if (!oldPtr)
			{
				if (oldSize != 0)
					return Err(Error("Reallocating from nullptr with old size > 0."));

				return AllocateBytes(newSize, alignment);
			}

			if (newSize == 0)
			{
				if (alignment > _spec.Alignment || _spec.Alignment % alignment != 0)
					return Err(Error("Reallocating with invalid alignment."));
				_pool.Deallocate(oldPtr);
				return Ok(nullptr);
			}

			if (oldSize > _spec.Size)
				return Err(Error("Old size is larger than pool's alloc spec size."));

			if (newSize > _spec.Size)
				return Err(Error("Size is larger than pool's alloc spec size."));

			if (alignment > _spec.Alignment || _spec.Alignment % alignment != 0)
				return Err(Error("Alignment does not match pool's alloc spec."));

			return Ok(oldPtr);
		}

		[[nodiscard]] AllocSpec GetAllocSpec() const
		{
			return _spec;
		}

	private:
		const AllocSpec _spec;
		MemoryPool _pool;
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
