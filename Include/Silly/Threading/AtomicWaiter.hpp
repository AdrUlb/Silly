#pragma once
#include <atomic>

#include "Silly/Cpu.hpp"
#include "Silly/Macros.hpp"

namespace Silly::Threading
{
	// The dummy waiter does not actually wait
	struct DummyAtomicWaiter
	{
		template<typename T> static void Wait(const std::atomic<T>&, const T, std::memory_order = std::memory_order_seq_cst) noexcept
		{
			Cpu::Pause();
		}
		template<typename T> static void NotifyOne(std::atomic<T>&) noexcept {}
		template<typename T> static void NotifyAll(std::atomic<T>&) noexcept {}

		static void Wait(const std::atomic_flag&, const bool, std::memory_order = std::memory_order_seq_cst) noexcept {}
		static void NotifyOne(std::atomic_flag&) noexcept {}
		static void NotifyAll(std::atomic_flag&) noexcept {}
	};

#if SILLY_HOSTED
	struct StdAtomicWaiter
	{
		template<typename T> static void Wait(const std::atomic<T>& state, const T value, std::memory_order memoryOrder = std::memory_order_seq_cst) noexcept
		{
			state.wait(value, memoryOrder);
		}

		template<typename T> static void NotifyOne(std::atomic<T>& state) noexcept
		{
			state.notify_one();
		}

		template<typename T> static void NotifyAll(std::atomic<T>& state) noexcept
		{
			state.notify_all();
		}

		static void Wait(const std::atomic_flag& state, const bool value, std::memory_order memoryOrder = std::memory_order_seq_cst) noexcept
		{
			state.wait(value, memoryOrder);
		}

		static void NotifyOne(std::atomic_flag& state) noexcept
		{
			state.notify_one();
		}

		static void NotifyAll(std::atomic_flag& state) noexcept
		{
			state.notify_all();
		}
	};
#endif

#if SILLY_HOSTED
	using AtomicWaiter = StdAtomicWaiter;
#else
	using AtomicWaiter = DummyAtomicWaiter;
#endif
}
