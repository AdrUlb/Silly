// Explicitly instantiate all (most) templates and includes every header file to catch any potential errors
// Really, this should be replaced with unit tests... but that's work!

#if !defined(NDEBUG)
#include <cstring>
#include "Silly/Box.hpp"
#include "Silly/Buffer.hpp"
#include "Silly/Cpu.hpp"
#include "Silly/Empty.hpp"
#include "Silly/Error.hpp"
#include "Silly/Extern.hpp"
#include "Silly/FlatMap.hpp"
#include "Silly/Formatter.hpp"
#include "Silly/IntrusiveList.hpp"
#include "Silly/KeyValuePair.hpp"
#include "Silly/LinkedList.hpp"
#include "Silly/List.hpp"
#include "Silly/Macros.hpp"
#include "Silly/Main.hpp"
#include "Silly/Option.hpp"
#include "Silly/PointerIterator.hpp"
#include "Silly/Ref.hpp"
#include "Silly/Result.hpp"
#include "Silly/Span.hpp"
#include "Silly/String.hpp"
#include "Silly/StringView.hpp"
#include "Silly/Utf8.hpp"
#include "Silly/WrappedRef.hpp"

#include "Silly/Hashing/Fnv1a64HashAlgorithm.hpp"
#include "Silly/Hashing/HashAlgorithm.hpp"
#include "Silly/Hashing/XXH64HashAlgorithm.hpp"

#include "Silly/Memory/Allocator.hpp"
#include "Silly/Memory/ArenaAllocator.hpp"
#include "Silly/Memory/HeapAllocator.hpp"
#include "Silly/Memory/MemoryPool.hpp"
#include "Silly/Memory/PoolAllocator.hpp"

#include "Silly/Threading/AtomicWaiter.hpp"
#include "Silly/Threading/DummyLock.hpp"
#include "Silly/Threading/Lockable.hpp"
#include "Silly/Threading/Mutex.hpp"
#include "Silly/Threading/ReaderWriterLock.hpp"
#include "Silly/Threading/SpinLock.hpp"

namespace Silly
{
	template class Box<int>;
	template class Buffer<int>;
	template class FlatMap<int, int>;
	template class KeyValuePair<int, int>;
	template class LinkedList<int>;
	template class List<int>;
	template class Option<int>;
	template class Option<void>;
	template class PointerIterator<int>;
	template class Ref<int>;
	template class Result<int, int>;
	template class Result<void, int>;
	template class Result<int, void>;
	template class Result<void, void>;
	template class Span<int>;
}
#endif
