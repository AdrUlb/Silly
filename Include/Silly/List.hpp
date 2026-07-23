#pragma once
#include "Span.hpp"
#include "Memory/Allocator.hpp"

namespace Silly
{
	template<typename T>
		requires(!std::is_const_v<T> && (std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>))
	class List
	{
	public:
		using iterator = PointerIterator<T>;
		using const_iterator = PointerIterator<T>;

		explicit List(Memory::Allocator& allocator) : _allocator(&allocator), _length(0) {}

		~List()
		{
			IGNORE(SetCapacity(0));
		}

		List(const List&) = delete;
		List& operator=(const List&) = delete;
		List(List&&) = delete;
		List& operator=(List&&) = delete;

		[[nodiscard]] iterator begin()
		{
			return iterator { _storage.AsPointer() };
		}

		[[nodiscard]] iterator end()
		{
			return iterator { _storage.AsPointer() + _length };
		}

		[[nodiscard]] const_iterator begin() const
		{
			return const_iterator { _storage.AsPointer() };
		}

		[[nodiscard]] const_iterator end() const
		{
			return const_iterator { _storage.AsPointer() + _length };
		}

		[[nodiscard]] size_t GetLength() const
		{
			return _length;
		}

		[[nodiscard]] size_t GetCapacity() const
		{
			return _storage.GetLength();
		}

		[[nodiscard]] bool IsEmpty() const
		{
			return _length == 0;
		}

		[[nodiscard]] std::span<const T> AsSpan() const
		{
			return _storage.Slice(0, _length);
		}

		[[nodiscard]] Result<void, Error> Add(const T& item)
			requires(std::is_copy_constructible_v<T>)
		{
			TRY(EnsureCapacity(GetLength() + 1));

#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(_storage.AsPointer() + _length, item);
				_length++;
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				return Err(Error("An exception occured while attempting to move the item."));
			}
#endif

			return Ok();
		}

		[[nodiscard]] Result<void, Error> Add(T&& item)
		{
			TRY(EnsureCapacity(GetLength() + 1));

#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(_storage.AsPointer() + _length, std::move_if_noexcept(item));
				_length++;
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				return Err(Error("An exception occured while attempting to move the item."));
			}
#endif

			return Ok();
		}

		template<typename... Args>
		[[nodiscard]] Result<void, Error> Add(std::in_place_t, Args&&... args)
		{
			TRY(EnsureCapacity(GetLength() + 1));

#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::construct_at(_storage.AsPointer() + _length, std::forward<Args>(args)...);
				_length++;
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				return Err(Error("An exception occured while attempting to construct the item."));
			}
#endif

			return Ok();
		}

		[[nodiscard]] Result<void, Error> AddRange(Span<const T> items)
			requires(std::is_copy_constructible_v<T>)
		{
			if (items.IsEmpty())
				return Ok();

			const auto itemsCount = items.GetLength();

			// Self-aliasing check
			{
				const auto oldLength = GetLength();
				const auto storageStart = _storage.AsPointer();
				const auto storageEnd = storageStart + oldLength;
				const auto itemsStart = items.AsPointer();

				if (itemsStart >= storageStart && itemsStart < storageEnd) // Aliasing!
				{
					const auto itemsOffset = itemsStart - storageStart;
					TRY(EnsureCapacity(oldLength + itemsCount));

					// Fix-up the items span
					items = { _storage.AsPointer() + itemsOffset, itemsCount };
				}
				else
				{
					TRY(EnsureCapacity(oldLength + itemsCount));
				}
			}

			size_t copyIndex = 0;

#if defined(__EXCEPTIONS)
			try
#endif
			{
				for (const auto& item : items)
				{
					std::construct_at(_storage.AsPointer() + _length + copyIndex, item);
					copyIndex++;
				}
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				_length += copyIndex;
				return Err(Error("An exception occured while copying items."));
			}
#endif

			_length += copyIndex;
			return Ok();
		}

		[[nodiscard]] Result<void, Error> RemoveRange(const size_t index, const size_t count)
		{
			if (count == 0)
				return Ok();

			const auto oldLength = GetLength();

			if (index >= oldLength)
				return Err(Error("The requested index was out of bounds."));

			if (index + count > oldLength)
				return Err(Error("The requested index and count were out of bounds."));

			const auto moveStart = _storage.AsPointer() + index + count;
			const auto moveEnd = _storage.AsPointer() + oldLength;
			const auto moveDest = _storage.AsPointer() + index;

#if defined(__EXCEPTIONS)
			try
#endif
			{
				std::move(moveStart, moveEnd, moveDest);
				std::destroy_n(_storage.AsPointer() + oldLength - count, count);
				_length -= count;
				return Ok();
			}
#if defined(__EXCEPTIONS)
			catch (...)
			{
				return Err(Error("An exception occured while attempting to move items."));
			}
#endif
		}

		[[nodiscard]] Result<void, Error> RemoveAt(const size_t index)
		{
			return RemoveRange(index, 1);
		}

		[[nodiscard]] Result<void, Error> SetCapacity(const size_t newCapacity)
			requires(std::is_trivially_copyable_v<T>) // If T is trivially copyable, just realloc the storage
		{
			if (newCapacity == GetCapacity())
				return Ok();

			if constexpr (!std::is_trivially_destructible_v<T>)
				if (newCapacity < _length)
					std::destroy_n(_storage.AsPointer() + newCapacity, _length - newCapacity);

			_storage = TRY(_allocator->ReallocateSpan(_storage, newCapacity));
			_length = std::min(_length, newCapacity);
			return Ok();
		}

		[[nodiscard]] Result<void, Error> SetCapacity(const size_t newCapacity)
		{
			if (newCapacity == GetCapacity())
				return Ok();

			// Create the new storage allocation, or provide an empty span if setting capacity to 0
			const auto newStorage = newCapacity == 0 ? Span<T> { } : TRY(_allocator->AllocateSpan<T>(newCapacity));

			// If neither the list nor the new allocation is empty,
			if (!IsEmpty() && !newStorage.IsEmpty())
			{
				// The number of copied elements; copies all old elements, unless the new capacity is smaller
				const auto copyCount = std::min(GetLength(), newCapacity);

				size_t copyIndex = 0;
#if defined(__EXCEPTIONS)
				try
#endif
				{
					while (copyIndex < copyCount)
					{
						std::construct_at(newStorage.AsPointer() + copyIndex, std::move_if_noexcept(_storage.AsPointer()[copyIndex]));
						copyIndex++;
					}
				}
#if defined(__EXCEPTIONS)
				catch (...) // T does not have a noexcept move constructor, a copy was performed instead (and failed)
				{
					// All original elements are still valid because a copy was perfomed, destroy the elements that were successfully copied and deallocate
					std::destroy_n(newStorage.AsPointer(), copyIndex);
					_allocator->DeallocateSpan(newStorage);
					return Err(Error("An exception occured while attempting to move the items."));
				}
#endif
			}

			if (!_storage.IsEmpty())
			{
				std::destroy_n(_storage.AsPointer(), GetLength());
				_allocator->DeallocateSpan(_storage);
			}
			_storage = newStorage;
			_length = std::min(_length, newCapacity);
			return Ok();
		}

		[[nodiscard]] Result<void, Error> EnsureCapacity(const size_t requiredCapacity)
		{
			const auto capacity = GetCapacity();

			// The current capacity already matches or exceeds what is required
			if (capacity >= requiredCapacity)
				return Ok();

			// The ideal growth rate is about x1.5
			const auto idealCap = capacity == 0 ? 8 : capacity * 3 / 2;

			if (idealCap > requiredCapacity)
				if (SetCapacity(idealCap))
					return Ok();

			// Fall back to growing only by as much as is needed
			TRY(SetCapacity(requiredCapacity));
			return Ok();
		}

		[[nodiscard]] T& operator[](size_t index)
		{
			return _storage[index];
		}

		[[nodiscard]] const T& operator[](size_t index) const
		{
			return _storage[index];
		}

	private:
		Memory::Allocator* _allocator;
		Span<T> _storage = { };
		size_t _length;
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
