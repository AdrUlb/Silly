#include "Silly/Memory/MemoryPool.hpp"

namespace Silly::Memory
{
	[[nodiscard]] Result<void*, Error> MemoryPool::Allocate()
	{
		if (!_blocks)
			TRY(AllocateChunk());

		// After successfully allocating blocks, there must now be a block
		VERIFY(_blocks);

		const auto block = _blocks;
		_blocks = block->Next;

#if !defined(NDEBUG)
		_allocCount++;
#endif
		return Ok(block);
	}

	void MemoryPool::Deallocate(void* ptr)
	{
		if (!ptr)
			return;

		const auto block = static_cast<Block*>(ptr);
		block->Next = _blocks;
		_blocks = block;

#if !defined(NDEBUG)
		_allocCount--;
#endif
	}

	void MemoryPool::Reset()
	{
#if !defined(NDEBUG)
		VERIFY(_allocCount == 0);
#endif

		auto chunk = _chunks;
		while (chunk)
		{
			const auto next = chunk->Next;
			_allocator->DeallocateBytes(chunk, GetChunkSize(), _alignment);
			chunk = next;
		}

		_chunks = nullptr;
		_blocks = nullptr;
	}

	[[nodiscard]] Result<void, Error> MemoryPool::AllocateChunk()
	{
		const auto alloc = TRY(_allocator->AllocateBytes(GetChunkSize(), _alignment));

		const auto chunk = static_cast<Chunk*>(alloc);
		chunk->Next = _chunks;
		_chunks = chunk;

		for (size_t i = _blocksPerChunk; i > 0; i--)
		{
			const auto block = reinterpret_cast<Block*>(static_cast<std::byte*>(alloc) + GetHeaderSize() + (i - 1) * _blockSize);
			block->Next = _blocks;
			_blocks = block;
		}

		return Ok();
	}
}
