#pragma once
#include "Silly/Option.hpp"

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

	struct LockableBase
	{
		template<template <typename...> typename Guard = LockGuard, typename Self> requires (LockableGuard<Guard>)
		[[nodiscard]] auto Take(this Self&& self)
		{
			return Guard<std::remove_cvref_t<Self>>::Take(self);
		}

		template<template <typename...> typename Guard = LockGuard, typename Self> requires (LockableGuard<Guard>)
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

#if SILLY_GLOBAL
using namespace Silly;
#endif
