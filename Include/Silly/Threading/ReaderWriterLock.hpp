#pragma once
#include <atomic>

#include "Lockable.hpp"
#include "Silly/Threading/AtomicWaiter.hpp"

namespace Silly::Threading
{
	template<typename Waiter>
	class ReaderWriterLock
	{
		static constexpr uint32_t WRITER_WAITING = 1U << 31;
		static constexpr uint32_t WRITER_LOCKED = 1U << 30;
		static constexpr uint32_t READER_COUNT_MASK = ~(WRITER_WAITING | WRITER_LOCKED);

	public:
		class Reader : public LockableBase
		{
			friend ReaderWriterLock;

			explicit Reader(ReaderWriterLock* owner) : _owner(owner) {}

		public:
			[[nodiscard]] bool TryAcquire() noexcept
			{
				return _owner->TryAcquireRead();
			}

			void Acquire() noexcept
			{
				_owner->AcquireRead();
			}

			void Release() noexcept
			{
				_owner->ReleaseRead();
			}

		private:
			ReaderWriterLock* _owner;
		};

		class Writer : public LockableBase
		{
			friend ReaderWriterLock;

			explicit Writer(ReaderWriterLock* owner) : _owner(owner) {}

		public:
			[[nodiscard]] bool TryAcquire() noexcept
			{
				return _owner->TryAcquireWrite();
			}

			void Acquire() noexcept
			{
				_owner->AcquireWrite();
			}

			void Release() noexcept
			{
				_owner->ReleaseWrite();
			}

		private:
			ReaderWriterLock* _owner;
		};

	public:
		Reader AsReader()
		{
			return Reader(this);
		}

		Writer AsWriter()
		{
			return Writer(this);
		}

	private:
		bool TryAcquireRead() noexcept
		{
			// 1. Load state
			// 2. Check if a writer has the lock or is waiting for it, if so give up.
			// 3. Attempt to increment the number of readers, if this fails, go to step 2
			// 4. Profit

			auto value = _state.load(std::memory_order_relaxed);

			while (!(value & (WRITER_LOCKED | WRITER_WAITING)))
			{
				if (_state.compare_exchange_strong(value, value + 1, std::memory_order_acquire, std::memory_order_relaxed))
					return true;
			}

			return false;
		}

		void AcquireRead() noexcept
		{
			// 1. Load state
			// 2. Check if a writer has the lock or is waiting for it, if so wait.
			// 3. Attempt to increment the number of readers, if this fails, go to step 2
			// 4. Profit

			auto value = _state.load(std::memory_order_relaxed);

			while (true)
			{
				if (value & (WRITER_LOCKED | WRITER_WAITING))
				{
					Waiter::Wait(_state, value);

					value = _state.load(std::memory_order_relaxed);
					continue;
				}

				if (_state.compare_exchange_weak(value, value + 1, std::memory_order_acquire, std::memory_order_relaxed))
					break;
			}
		}

		void ReleaseRead() noexcept
		{
			const auto old = _state.fetch_sub(1, std::memory_order_release);
			const auto readerCount = old & READER_COUNT_MASK;

			// We were the last reader and there is a writer waiting for a notification, notify it!
			if (readerCount == 1 && (old & WRITER_WAITING))
				Waiter::NotifyAll(_state);
		}

		bool TryAcquireWrite() noexcept
		{
			auto value = _state.load(std::memory_order_relaxed);

			if (value & (READER_COUNT_MASK | WRITER_LOCKED | WRITER_WAITING))
				return false;

			return _state.compare_exchange_strong(value, value | WRITER_LOCKED, std::memory_order_acquire, std::memory_order_relaxed);
		}

		void AcquireWrite() noexcept
		{
			auto value = _state.load(std::memory_order_relaxed);

			while (true)
			{
				if (value & WRITER_WAITING)
				{
					Waiter::Wait(_state, value);

					value = _state.load(std::memory_order_relaxed);
					continue;
				}

				if (_state.compare_exchange_weak(value, value | WRITER_WAITING, std::memory_order_relaxed))
					break;
			}

			while (true)
			{
				const auto readerCount = value & READER_COUNT_MASK;

				if (readerCount > 0 || value & WRITER_LOCKED) // If the lock is currently held by ANYONE AT ALL
				{
					Waiter::Wait(_state, value);

					value = _state.load(std::memory_order_relaxed);
					continue;
				}

				if (_state.compare_exchange_weak(value, value & ~WRITER_WAITING | WRITER_LOCKED, std::memory_order_acquire, std::memory_order_relaxed))
					break;
			}
		}

		void ReleaseWrite() noexcept
		{
			const auto old = _state.fetch_and(~WRITER_LOCKED, std::memory_order_release);
			Waiter::NotifyAll(_state);
		}

		std::atomic<uint32_t> _state { 0 };

		static_assert(Lockable<Reader>);
		static_assert(Lockable<Writer>);
	};
}
