#pragma once
#include "Allocator.hpp"

namespace Silly::Memory
{
	class HeapAllocator final : public Allocator
	{
	public:
		[[nodiscard]] Result<void*, Error> AllocateBytes(const size_t size, const size_t alignment) noexcept override
		{
			if (!IsValidAlignment(alignment))
				return Err(Error("Invalid alignment requested."));

			const auto ptr = operator new(size, static_cast<std::align_val_t>(alignment), std::nothrow);
			if (!ptr)
				return Err(Error("Heap allocation failed."));

			return Ok(ptr);
		}

		void DeallocateBytes(void* ptr, const size_t size, const size_t alignment) noexcept override
		{
			operator delete(ptr, size, static_cast<std::align_val_t>(alignment));
		}

		static HeapAllocator& GetGlobal()
		{
			static HeapAllocator instance;
			return instance;
		}
	};
}
