#pragma once
#include "Silly/Cpu.hpp"
#include "Silly/Empty.hpp"
#include "Silly/Error.hpp"
#include "Silly/Result.hpp"

namespace Silly::Threading
{
	enum IrqSafeTag { IrqSafe };

	enum IrqUnsafeTag { IrqUnsafe };

	struct LockableBase;

	template<typename T>
	concept Lockable = requires(T lock)
	{
		{ lock.TryAcquire() } -> std::same_as<bool>;
		{ lock.Acquire() } -> std::same_as<void>;
		{ lock.Release() } -> std::same_as<void>;
	} && std::is_base_of_v<LockableBase, T>;

	template<bool IsIrqSafe, Lockable Lock>
	class LockGuard
	{
		friend LockableBase;

	private:
		explicit LockGuard(Lock& lock) noexcept
			requires(!IsIrqSafe)
			: _lock(&lock) {}

		explicit LockGuard(Lock& lock, Cpu::IrqGuard&& irqGuard) noexcept
			requires(IsIrqSafe)
			: _lock(&lock), _irqGuard(std::move(irqGuard)) {}

	public:
		~LockGuard()
		{
			if (_lock)
				_lock->Release();
		}

		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;

		LockGuard(LockGuard&& other) noexcept
			: _lock(std::exchange(other._lock, nullptr)), _irqGuard(std::move(other._irqGuard)) {}

		LockGuard& operator=(LockGuard&& other) noexcept
		{
			if (&other == this)
				return *this;

			if (_lock)
				_lock->Release();

			if constexpr (IsIrqSafe)
				_irqGuard = std::move(other._irqGuard);

			_lock = std::exchange(other._lock, nullptr);
			return *this;
		}

	private:
		Lock* _lock;
		[[no_unique_address]] std::conditional_t<IsIrqSafe, Cpu::IrqGuard, Empty> _irqGuard;
	};

	struct LockableBase
	{
		template<Lockable Self>
		[[nodiscard]] auto Hold(this Self& self, IrqSafeTag)
		{
			auto irqGuard = Cpu::IrqGuard::Disable();
			self.Acquire();
			return LockGuard<true, Self>(self, std::move(irqGuard));
		}

		template<Lockable Self>
		[[nodiscard]] auto Hold(this Self& self, IrqUnsafeTag = IrqUnsafe)
		{
			self.Acquire();
			return LockGuard<false, Self>(self);
		}

		template<Lockable Self>
		[[nodiscard]] Result<LockGuard<true, Self>, Error> TryHold(this Self& self, IrqSafeTag)
		{
			auto irqGuard = Cpu::IrqGuard::Disable();

			if (self.TryAcquire())
				return Ok(LockGuard<true, Self>(self, std::move(irqGuard)));

			return Err(Error("Failed to acquire the lock."));
		}

		template<Lockable Self>
		[[nodiscard]] Result<LockGuard<false, Self>, Error> TryHold(this Self& self, IrqUnsafeTag = IrqUnsafe)
		{
			if (self.TryAcquire())
				return Ok(LockGuard<false, Self>(self));

			return Err(Error("Failed to acquire the lock."));
		}
	};
}
