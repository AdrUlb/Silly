#pragma once
#include "AtomicWaiter.hpp"
#include "Lockable.hpp"
#include "Silly/Cpu.hpp"

namespace Silly::Threading
{
	template<typename Waiter>
	class Mutex : public LockableBase
	{
	public:
		[[nodiscard]] bool TryAcquire() noexcept
		{
			return !_flag.test(std::memory_order_relaxed) && !_flag.test_and_set(std::memory_order_acquire);
		}

		void Acquire() noexcept
		{
			// Interrupts CANNOT be disabled, use a spin-lock instead.
			VERIFY(Cpu::GetInterruptsEnabled());

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

	using SpinLock = Mutex<SpinWaiter>;
	static_assert(Lockable<SpinLock>);
}
