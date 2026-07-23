#pragma once
#include <atomic>

#include "Silly/Cpu.hpp"
#include "Silly/Macros.hpp"

namespace Silly::Threading
{
	// The dummy waiter does not actually wait
	template<typename T>
	concept Waiter = requires(std::atomic<T>& atomic, std::atomic_flag flag, std::memory_order memoryOrder, T value, bool flagValue)
	{
		{ T::Wait(atomic, value) } -> std::same_as<void>;
		{ T::Wait(atomic, value, memoryOrder) } -> std::same_as<void>;
		{ T::NotifyOne(atomic) } -> std::same_as<void>;
		{ T::NotifyAll(atomic) } -> std::same_as<void>;

		{ T::Wait(flag, flagValue) } -> std::same_as<void>;
		{ T::Wait(flag, flagValue, memoryOrder) } -> std::same_as<void>;
		{ T::NotifyOne(flag) } -> std::same_as<void>;
		{ T::NotifyAll(flag) } -> std::same_as<void>;
	};

	// The dummy waiter does not actually wait
	struct SpinWaiter
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

	static_assert(Waiter<SpinWaiter>);

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

	static_assert(Waiter<StdAtomicWaiter>);
#endif
}
