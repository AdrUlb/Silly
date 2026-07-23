#pragma once
#include "Lockable.hpp"
#include "Waiter.hpp"

namespace Silly::Threading
{
	template<Waiter Waiter>
	class Mutex : public LockableBase
	{
	public:
		[[nodiscard]] bool TryAcquire() noexcept
		{
			return !_flag.test(std::memory_order_relaxed) && !_flag.test_and_set(std::memory_order_acquire);
		}

		void Acquire() noexcept
		{
			while (!TryAcquire())
				Waiter::Wait(_flag, true, std::memory_order_relaxed);
		}

		void Release() noexcept
		{
			_flag.clear(std::memory_order_release);
			Waiter::NotifyOne(_flag);
		}

	private:
		std::atomic_flag _flag { false };
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
