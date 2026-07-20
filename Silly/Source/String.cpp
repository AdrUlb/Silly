#include "Silly/String.hpp"

namespace Silly
{
	[[nodiscard]] Result<void, Error> String::SetLength(const size_t newLength)
	{
		TRY(EnsureCapacity(newLength));
		_lengthAndFlags = (_lengthAndFlags & LARGE_FLAG) | newLength;
		GetStorage()[newLength] = 0;
		return Ok();
	}

	[[nodiscard]] Result<void, Error> String::SetCapacity(const size_t newCapacity)
	{
		if (newCapacity == 0)
		{
			if (!IsSmall())
				_allocator->DeallocateSpan(_storage.Large);

			_lengthAndFlags = 0;
			_storage.Small[0] = 0;
			return Ok();
		}

		const auto oldIsSmall = IsSmall();
		const auto newIsSmall = newCapacity <= SMALL_CAPACITY - 1;

		const auto newLength = std::min(GetLength(), newCapacity);

		if (oldIsSmall && newIsSmall)
		{
			_lengthAndFlags = newLength;
			_storage.Small[newLength] = '\0';
			return Ok();
		}

		if (!oldIsSmall && !newIsSmall)
		{
			_storage.Large = TRY(_allocator->ReallocateSpan<char>(_storage.Large, newCapacity + 1));
			_storage.Large[newLength] = 0;
			_lengthAndFlags = newLength | LARGE_FLAG;
			return Ok();
		}

		const auto oldStorage = GetStorage();
		const auto newStorage = newIsSmall ? Span(_storage.Small, newCapacity + 1) : TRY(_allocator->AllocateSpan<char>(newCapacity + 1));
		oldStorage.Slice(0, newLength).CopyTo(newStorage);
		newStorage[newLength] = 0;

		VERIFY(!oldIsSmall == newIsSmall);
		if (!oldIsSmall)
		{
			_allocator->DeallocateSpan(oldStorage);
			_lengthAndFlags = newLength;
		}
		else
		{
			_lengthAndFlags = newLength | LARGE_FLAG;
			_storage.Large = newStorage;
		}

		return Ok();
	}

	[[nodiscard]] Result<void, Error> String::EnsureCapacity(const size_t requiredCapacity)
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

	[[nodiscard]] Result<void, Error> String::Assign(const StringView view)
	{
		// NOTE: even if we're aliased, view's length must be <= our length so SetLength will never reallocate here
		TRY(EnsureCapacity(view.GetLength())); // First we make sure the storage can hold the new string data
		view.CopyTo(GetStorage()); // Then we copy it over
		TRY(SetLength(view.GetLength())); // And finally update length and write the null terminator
		return Ok();
	}

	[[nodiscard]] Result<void, Error> String::Append(StringView view)
	{
		// Get self, as a StringView
		const auto self = AsView();

		// Check if the StringView to append is part of this string instance (aliasing)
		const auto aliased = view.AsPointer() >= self.AsPointer() && view.AsPointer() < self.AsPointer() + GetLength();

		const auto offset = view.AsPointer() - self.AsPointer();

		// Set new length, this may increase capacity if needed (and potentially reallocate)
		TRY(SetLength(self.GetLength() + view.GetLength()));

		if (aliased)
		{
			// Changing the capacity while trying to append part of ourselves to ourselves might have invalidated the StringView
			// Update it with the new pointer
			const auto newSelf = AsView();
			view = StringView(offset + newSelf.AsPointer(), view.GetLength());
		}

		// Copy the actual string data
		view.CopyTo(GetStorage().Slice(self.GetLength()));
		return Ok();
	}
}
