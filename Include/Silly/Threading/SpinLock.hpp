#pragma once
#include <atomic>
#include "Lockable.hpp"
#include "Silly/Cpu.hpp"

namespace Silly::Threading
{
	class SpinLock : public LockableBase
	{
	public:
		[[nodiscard]] bool TryAcquire() noexcept
		{
			return !_flag.test(std::memory_order_relaxed) && !_flag.test_and_set(std::memory_order_acquire);
		}

		void Acquire() noexcept
		{
			while (!TryAcquire())
				Cpu::Pause();
		}

		void Release() noexcept
		{
			_flag.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag _flag { false };
	};

	static_assert(Lockable<SpinLock>);
}
