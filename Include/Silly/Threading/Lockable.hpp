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
	struct LockableArchetype;

	template<typename T>
	concept Lockable = requires(T lock)
	{
		{ lock.TryAcquire() } -> std::same_as<bool>;
		{ lock.Acquire() } -> std::same_as<void>;
		{ lock.Release() } -> std::same_as<void>;
	} && std::is_base_of_v<LockableBase, T>;

	template<template <typename...> typename T>
	concept LockableGuard = requires(LockableArchetype& lock)
	{
		{ T<LockableArchetype>::TryTake(lock) } -> std::same_as<Option<T<LockableArchetype>>>;
		{ T<LockableArchetype>::Take(lock) } -> std::same_as<T<LockableArchetype>>;
	};

	template<Lockable Lock>
	class LockGuard
	{
		friend LockableBase;

	private:
		explicit LockGuard(Lock& lock) noexcept
			: _lock(&lock) {}

	public:
		~LockGuard()
		{
			if (_lock)
				_lock->Release();
		}

		LockGuard(const LockGuard&) = delete;
		LockGuard& operator=(const LockGuard&) = delete;

		LockGuard(LockGuard&& other) noexcept
			: _lock(std::exchange(other._lock, nullptr)) {}

		LockGuard& operator=(LockGuard&& other) noexcept
		{
			if (&other == this)
				return *this;

			if (_lock)
				_lock->Release();

			_lock = std::exchange(other._lock, nullptr);
			return *this;
		}

		[[nodiscard]] static LockGuard Take(Lock& lock)
		{
			lock.Acquire();
			return LockGuard(lock);
		}

		[[nodiscard]] static Option<LockGuard> TryTake(Lock& lock)
		{
			if (lock.TryAcquire())
				return Some(LockGuard(lock));

			return None;
		}

	private:
		Lock* _lock;
	};

	template<Lockable Lock>
	class IrqLockGuard
	{
		friend LockableBase;

	private:
		explicit IrqLockGuard(LockGuard<Lock>&& lockGuard, Cpu::IrqGuard&& irqGuard) noexcept
			: _irqGuard(std::move(irqGuard)), _lock(std::move(lockGuard)) {}

	public:
		[[nodiscard]] static IrqLockGuard Take(Lock& lock)
		{
			return IrqLockGuard(LockGuard<Lock>::Take(lock), Cpu::IrqGuard::Disable());
		}

		[[nodiscard]] static Option<IrqLockGuard> TryTake(Lock& lock)
		{
			auto guard = LockGuard<Lock>::TryTake(lock);
			if (!guard)
				return None;

			return Some(IrqLockGuard(std::move(guard).Unwrap(), Cpu::IrqGuard::Disable()));
		}

	private:
		// NOTE: the order here is important: the lock and guard are released in *reverse order of declaration here*
		Cpu::IrqGuard _irqGuard;
		LockGuard<Lock> _lock;
	};

	struct LockableBase
	{
		template<template <typename...> typename Guard, typename Self> requires (LockableGuard<Guard>)
		[[nodiscard]] auto Take(this Self&& self)
		{
			return Guard<std::remove_cvref_t<Self>>::Take(self);
		}

		template<template <typename...> typename Guard, typename Self> requires (LockableGuard<Guard>)
		[[nodiscard]] auto TryTake(this Self&& self)
		{
			return Guard<std::remove_cvref_t<Self>>::TryTake(self);
		}
	};

	// The Lockable archetype, at minimum every Lockable must implement this interface
	struct LockableArchetype : LockableBase
	{
		[[nodiscard]] bool TryAcquire();
		void Acquire();
		void Release();
	};

	static_assert(LockableGuard<LockGuard>);
}
