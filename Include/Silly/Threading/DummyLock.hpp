#pragma once
#include "Lockable.hpp"

namespace Silly::Threading
{
	class DummyLock : public LockableBase
	{
	public:
		[[nodiscard]] bool TryAcquire() noexcept { return true; }
		void Acquire() noexcept {}
		void Release() noexcept {}
	};

	static_assert(Lockable<DummyLock>);
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
