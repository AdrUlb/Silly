#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>

#include "Macros.hpp"

namespace Silly::Cpu
{
	FORCE_INLINE void Pause()
	{
#if defined(__x86_64__) || defined(__i386__)
		asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
		asm volatile("yield" ::: "memory");
#else
#warning No architecture-specific CPU pause instruction has been specified.
#endif
	}

	FORCE_INLINE size_t GetFlags()
	{
#if defined(__x86_64__) || defined(__i386__)
		size_t flags = 0;
		asm volatile(
			"pushf\n"
			"pop %0"
			: "=r"(flags)
		);
		return flags;
#else
#error No architecture-specific instruction for getting CPU flags has been specified.
#endif
	}

#if defined(KERNEL)
	FORCE_INLINE constexpr bool GetInterruptsEnabled() { return true; }
	FORCE_INLINE void EnableInterrupts() {}
	FORCE_INLINE void DisableInterrupts() {}
#else
	FORCE_INLINE bool GetInterruptsEnabled()
	{
#if defined(__x86_64__) || defined(__i386__)
		return GetFlags() & (1 << 9);
#else
#error No architecture-specific instruction for determining the interrupt state has been specified.
#endif
	}

	FORCE_INLINE void EnableInterrupts()
	{
#if defined(__x86_64__) || defined(__i386__)
		asm volatile("sti" ::: "memory");
#else
#error No architecture-specific instruction for enabling interrupts has been specified.
#endif
	}

	FORCE_INLINE void DisableInterrupts()
	{
#if defined(__x86_64__) || defined(__i386__)
		asm volatile("cli": : : "memory");
#else
#error No architecture-specific instruction for disabling interrupts has been specified.
#endif
	}
#endif

	class IrqGuard
	{
		enum class Action : uint8_t
		{
			None,
			Enable,
			Disable
		};

	public:
		IrqGuard(const IrqGuard&) = delete;
		IrqGuard& operator=(const IrqGuard&) = delete;

		IrqGuard(IrqGuard&& other) noexcept : _action(std::exchange(other._action, Action::None)) {}

		IrqGuard& operator=(IrqGuard&& other) noexcept
		{
			if (&other == this)
				return *this;

			Restore();
			_action = std::exchange(other._action, Action::None);

			return *this;
		}

		~IrqGuard()
		{
			Restore();
		}

		static IrqGuard Enable()
		{
			const auto action = GetInterruptsEnabled() ? Action::Enable : Action::Disable;
			EnableInterrupts();
			return IrqGuard { action };
		}

		static IrqGuard Disable()
		{
			const auto action = GetInterruptsEnabled() ? Action::Enable : Action::Disable;
			DisableInterrupts();
			return IrqGuard { action };
		}

	private:
		void Restore()
		{
			switch (_action)
			{
				case Action::None:
					break;
				case Action::Enable:
					EnableInterrupts();
					break;
				case Action::Disable:
					DisableInterrupts();
					break;
			}
		}

		explicit IrqGuard(const Action action) : _action(action) {}

		Action _action;
	};
}
