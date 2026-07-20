#pragma once
#include <atomic>

#include "Memory/Allocator.hpp"

namespace Silly
{
	class RefControlBlock
	{
		// Strong references prevent the object from being destroyed
		// Weak references prevent the wrapper from being destroyed (so it can still be queried as long as weak references exist)
		// All strong references collectively hold a weak reference keeping the wrapper alive as long as the object itself is alive
	public:
		virtual ~RefControlBlock() = default;

		void Retain() noexcept
		{
			// Increment the reference count
			// It does not matter when this happens relative to other operations on this thread so use relaxed ordering
			_refCount.fetch_add(1, std::memory_order_relaxed);
		}

		void Release() noexcept
		{
			// Decrement the reference count
			// We must ensure that any modifications to the object on this thread happen *before* we allow a different thread to destroy the object
			// Using release order synchronizes with the acquire fence on the destroying thread and ensures all of our writes happened before the destructor runs
			const auto dead = _refCount.fetch_sub(1, std::memory_order_release) == 1;
			if (!dead)
				return;

			// Ensure that we observe all modifications by every thread that released object before calling the destructor
			std::atomic_thread_fence(std::memory_order_acquire);
			DestroyObject();

			// Finally release the weak reference that all strong references collectively hold
			ReleaseWeak();
		}

		void RetainWeak() noexcept
		{
			// Increment the weak reference count
			_weakCount.fetch_add(1, std::memory_order_relaxed);
		}

		void ReleaseWeak() noexcept
		{
			// Decrement the weak reference count
			// Weak references can't access the object so why still use memory_order_release and a fence?
			// Because we must ensure the object's destructor ran before the wrapper is destroyed!
			// Lucky for us, all strong references collectively hold a weak reference which is released after the destructor runs
			const auto dead = _weakCount.fetch_sub(1, std::memory_order_release) == 1;
			if (!dead)
				return;

			std::atomic_thread_fence(std::memory_order_acquire);
			DestroyWrapper();
		}

		[[nodiscard]] bool TryPromote() noexcept
		{
			auto count = _refCount.load(std::memory_order_relaxed);

			while (count > 0) // Stop trying to promote if the count hits 0
			{
				// Attempt to increment the count
				if (_refCount.compare_exchange_weak(count, count + 1, std::memory_order_relaxed))
					return true;
			}

			// The object is already dead :(
			return false;
		}

		[[nodiscard]] uint32_t GetRefCount() const noexcept
		{
			// Get the current ref count
			// Using acquire order here synchronizes with the decrement in Release()
			// This ensures that all writes by all threads which have since released the object are observable
			return _refCount.load(std::memory_order_acquire);
		}

	protected:
		virtual void DestroyObject() noexcept = 0;
		virtual void DestroyWrapper() noexcept = 0;

	private:
		std::atomic<uint32_t> _refCount { 1 };
		std::atomic<uint32_t> _weakCount { 1 };
	};

	template<typename T>
	class Ref
	{
		class OwnedControlBlock final : public RefControlBlock
		{
		public:
			template<typename... Args>
			OwnedControlBlock(Memory::Allocator& allocator, Args&&... args) : _allocator(&allocator), _storage(std::in_place_index<1>, std::forward<Args>(args)...) {}

			T* GetPointer() noexcept
			{
				return &std::get<1>(_storage);
			}

		protected:
			void DestroyObject() noexcept override
			{
				_storage.template emplace<0>();
			}

			void DestroyWrapper() noexcept override
			{
				// Copy the allocator pointer into the current scope before calling our own destructor so we can still delete ourselves
				const auto allocator = _allocator;
				allocator->Delete(this);
			}

			Memory::Allocator* _allocator;
			std::variant<char, T> _storage; // Use a variant for manual lifetime management
		};

		class AdoptedControlBlock final : public RefControlBlock
		{
		public:
			AdoptedControlBlock(Memory::Allocator& cbAllocator, T* ptr, Memory::Allocator& ptrAllocator)
				: _cbAllocator(&cbAllocator), _ptr(ptr), _ptrAllocator(&ptrAllocator) {}

			T* GetPointer() noexcept
			{
				return _ptr;
			}

		protected:
			void DestroyObject() noexcept override
			{
				_ptrAllocator->Delete(_ptr);
			}

			void DestroyWrapper() noexcept override
			{
				_cbAllocator->Delete(this);
			}

		private:
			Memory::Allocator* _cbAllocator;
			T* _ptr;
			Memory::Allocator* _ptrAllocator;
		};

		template<typename> friend class Ref;
		template<typename> friend class WeakRef;

	public:
		Ref() noexcept : _controlBlock(nullptr), _ptr(nullptr) {}
		Ref(std::nullptr_t) noexcept : _controlBlock(nullptr), _ptr(nullptr) {}

		~Ref()
		{
			if (_controlBlock)
				_controlBlock->Release();
		}

		Ref(const Ref& other) noexcept : _controlBlock(other._controlBlock), _ptr(other._ptr)
		{
			if (_controlBlock)
				_controlBlock->Retain();
		}

		Ref& operator=(const Ref& other) noexcept
		{
			if (&other == this)
				return *this;

			if (other._controlBlock == _controlBlock) // Same control block, only have to update the pointer
			{
				_ptr = other._ptr;
				return *this;
			}

			if (_controlBlock)
				_controlBlock->Release();

			_ptr = other._ptr;
			_controlBlock = other._controlBlock;

			if (_controlBlock)
				_controlBlock->Retain();

			return *this;
		}

		Ref(Ref&& other) noexcept : _controlBlock(std::exchange(other._controlBlock, nullptr)), _ptr(std::exchange(other._ptr, nullptr)) {}

		Ref& operator=(Ref&& other) noexcept
		{
			if (&other == this)
				return *this;

			if (_controlBlock)
				_controlBlock->Release();

			_ptr = std::exchange(other._ptr, nullptr);
			_controlBlock = std::exchange(other._controlBlock, nullptr);

			return *this;
		}

		// Upcasting constructor
		template<typename U>
		Ref(const Ref<U>& derived) noexcept
			requires(std::is_convertible_v<U*, T*>) : _controlBlock(derived._controlBlock), _ptr(derived._ptr)
		{
			if (_controlBlock)
				_controlBlock->Retain();
		}

		// Upcasting move constructor
		template<typename U>
		Ref(Ref<U>&& other) noexcept
			requires(std::is_convertible_v<U*, T*>)
			: _controlBlock(std::exchange(other._controlBlock, nullptr)), _ptr(std::exchange(other._ptr, nullptr)) {}

		T* operator->() const { return _ptr; }
		T& operator*() const { return *_ptr; }
		explicit operator bool() const { return _controlBlock != nullptr; }

		bool operator==(const Ref& other) const { return _controlBlock == other._controlBlock; }
		bool operator!=(const Ref& other) const { return _controlBlock != other._controlBlock; }
		bool operator==(std::nullptr_t) const { return _controlBlock == nullptr; }
		bool operator!=(std::nullptr_t) const { return _controlBlock != nullptr; }

		template<typename... Args>
		[[nodiscard]] static Result<Ref, Error> New(Memory::Allocator& allocator, Args&&... args)
		{
			const auto cb = TRY(allocator.Allocate<OwnedControlBlock>());
#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(cb, allocator, std::forward<Args>(args)...);
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				allocator.Deallocate(cb);
				return Err(Error("An exception occured while constructing the object."));
			}
#endif
			return Ok(Ref { cb, cb->GetPointer() });
		}

		[[nodiscard]] static Result<Ref, Error> Adopt(Memory::Allocator& cbAllocator, T* ptr, Memory::Allocator& ptrAllocator)
		{
			const auto cb = TRY(cbAllocator.Allocate<AdoptedControlBlock>());
#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(cb, cbAllocator, ptr, ptrAllocator);
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				cbAllocator.Deallocate(cb);
				return Err(Error("An exception occured while constructing the control block."));
			}
#endif
			return Ok(Ref { cb, cb->GetPointer() });
		}

	private:
		Ref(RefControlBlock* controlBlock, T* ptr) noexcept : _controlBlock(controlBlock), _ptr(ptr) {}

		RefControlBlock* _controlBlock;
		T* _ptr;
	};

	template<typename T>
	class WeakRef
	{
		template<typename> friend class WeakRef;

	public:
		WeakRef() noexcept : _controlBlock(nullptr), _ptr(nullptr) {}
		WeakRef(std::nullptr_t) noexcept : _controlBlock(nullptr), _ptr(nullptr) {}

		~WeakRef()
		{
			if (_controlBlock)
				_controlBlock->ReleaseWeak();
		}

		WeakRef(const WeakRef& other) noexcept : _controlBlock(other._controlBlock), _ptr(other._ptr)
		{
			if (_controlBlock)
				_controlBlock->RetainWeak();
		}

		WeakRef& operator=(const WeakRef& other) noexcept
		{
			if (&other == this)
				return *this;

			if (other._controlBlock == _controlBlock) // Same control block, only have to update the pointer
			{
				_ptr = other._ptr;
				return *this;
			}

			if (_controlBlock)
				_controlBlock->ReleaseWeak();

			_ptr = other._ptr;
			_controlBlock = other._controlBlock;

			if (_controlBlock)
				_controlBlock->RetainWeak();

			return *this;
		}

		WeakRef(WeakRef&& other) noexcept : _controlBlock(std::exchange(other._controlBlock, nullptr)), _ptr(std::exchange(other._ptr, nullptr)) {}

		WeakRef& operator=(WeakRef&& other) noexcept
		{
			if (&other == this)
				return *this;

			if (_controlBlock)
				_controlBlock->ReleaseWeak();

			_ptr = std::exchange(other._ptr, nullptr);
			_controlBlock = std::exchange(other._controlBlock, nullptr);

			return *this;
		}

		WeakRef(const Ref<T>& ref) : _controlBlock(ref._controlBlock), _ptr(ref._ptr)
		{
			if (_controlBlock)
				_controlBlock->RetainWeak();
		}

		WeakRef& operator=(const Ref<T>& ref) noexcept
		{
			if (ref._controlBlock == _controlBlock) // Same control block, only have to update the pointer
			{
				_ptr = ref._ptr;
				return *this;
			}

			if (_controlBlock)
				_controlBlock->ReleaseWeak();

			_ptr = ref._ptr;
			_controlBlock = ref._controlBlock;

			if (_controlBlock)
				_controlBlock->RetainWeak();

			return *this;
		}

		// Upcasting constructor
		template<typename U>
		WeakRef(const WeakRef<U>& derived) noexcept
			requires(std::is_convertible_v<U*, T*>) : _controlBlock(derived._controlBlock), _ptr(derived._ptr)
		{
			if (_controlBlock)
				_controlBlock->RetainWeak();
		}

		// Upcasting move constructor
		template<typename U>
		WeakRef(WeakRef<U>&& other) noexcept
			requires(std::is_convertible_v<U*, T*>)
			: _controlBlock(std::exchange(other._controlBlock, nullptr)), _ptr(std::exchange(other._ptr, nullptr)) {}

		[[nodiscard]] bool IsAlive() const { return _controlBlock != nullptr && _controlBlock->GetRefCount() != 0; }

		[[nodiscard]] Option<Ref<T>> TryPromote() const noexcept
		{
			if (_controlBlock && _controlBlock->TryPromote())
				return Some(Ref<T> { _controlBlock, _ptr });

			return None;
		}

		bool operator==(const WeakRef& other) const { return _controlBlock == other._controlBlock; }
		bool operator!=(const WeakRef& other) const { return _controlBlock != other._controlBlock; }
		bool operator==(std::nullptr_t) const { return _controlBlock == nullptr; }
		bool operator!=(std::nullptr_t) const { return _controlBlock != nullptr; }

	private:
		RefControlBlock* _controlBlock;
		T* _ptr;
	};
}
