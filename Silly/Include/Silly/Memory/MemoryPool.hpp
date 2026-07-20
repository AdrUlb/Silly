#pragma once
#include "Allocator.hpp"

namespace Silly::Memory
{
	class MemoryPool
	{
		struct Block
		{
			Block* Next;
		};

		struct Chunk
		{
			Chunk* Next;
		};

	public:
		MemoryPool(Allocator& allocator, const size_t blockSize, const size_t blocksPerChunk, const size_t alignment)
			: _allocator(&allocator),
			  _blockSize(AlignUp(std::max(blockSize, sizeof(Block)), alignment)),
			  _blocksPerChunk(blocksPerChunk),
			  _alignment(alignment)
		{
			VERIFY(IsValidAlignment(alignment));
			VERIFY(blocksPerChunk != 0);
		}

		~MemoryPool()
		{
			Reset();
		}

		MemoryPool(const MemoryPool&) = delete;
		MemoryPool& operator=(const MemoryPool&) = delete;

		MemoryPool(MemoryPool&& other) noexcept
			: _allocator(other._allocator),
			  _blockSize(other._blockSize),
			  _blocksPerChunk(other._blocksPerChunk),
			  _alignment(other._alignment),
			  _chunks(std::exchange(other._chunks, nullptr)),
			  _blocks(std::exchange(other._blocks, nullptr))
#if !defined(NDEBUG)
			  , _allocCount(std::exchange(other._allocCount, 0))
#endif
		{}

		MemoryPool& operator=(MemoryPool&& other) noexcept
		{
			if (&other == this)
				return *this;

			Reset();
			_allocator = std::exchange(other._allocator, nullptr);
			_blockSize = std::exchange(other._blockSize, 0);
			_blocksPerChunk = std::exchange(other._blocksPerChunk, 0);
			_alignment = std::exchange(other._alignment, 0);
			_chunks = std::exchange(other._chunks, nullptr);
			_blocks = std::exchange(other._blocks, nullptr);
#if !defined(NDEBUG)
			_allocCount = std::exchange(other._allocCount, 0);
#endif
			return *this;
		}

		[[nodiscard]] Result<void*, Error> Allocate();
		void Deallocate(void* ptr);

	private:
		void Reset();

		[[nodiscard]] constexpr FORCE_INLINE size_t GetHeaderSize() const
		{
			return AlignUp(sizeof(Chunk), _alignment);
		}

		[[nodiscard]] constexpr FORCE_INLINE size_t GetChunkSize() const
		{
			return GetHeaderSize() + _blocksPerChunk * _blockSize;
		}

		[[nodiscard]] FORCE_INLINE Result<void, Error> AllocateChunk();

		Allocator* _allocator;
		size_t _blockSize, _blocksPerChunk, _alignment;
		Chunk* _chunks { nullptr };
		Block* _blocks { nullptr };
#if !defined(NDEBUG)
		size_t _allocCount = 0;
#endif
	};
}
