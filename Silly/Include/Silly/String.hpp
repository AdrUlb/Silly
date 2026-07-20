#pragma once
#include <climits>
#include <compare>

#include "Span.hpp"
#include "StringView.hpp"
#include "Memory/Allocator.hpp"

namespace Silly
{
	class String
	{
		// Stored in the length field, indicates whether SSO is active or not
		static constexpr size_t LARGE_FLAG = 1ULL << (sizeof(size_t) * CHAR_BIT - 1);

		// The capacity of a small string (including the null terminator)
		static constexpr size_t SMALL_CAPACITY = sizeof(Span<char>);

	public:
		explicit constexpr String(Memory::Allocator& allocator) : _allocator(&allocator), _storage({ }) {}

		~String()
		{
			IGNORE(SetCapacity(0));
		}

		// Allocations may fail, therefore no copy constructors
		String(const String&) = delete;

		String& operator=(const String&) = delete;

		constexpr String(String&& other) noexcept
			: _allocator(other._allocator),
			  _lengthAndFlags(std::exchange(other._lengthAndFlags, 0)),
			  _storage(other._storage)
		{
			other._storage.Small[0] = '\0';
		}

		String& operator=(String&& other) noexcept
		{
			if (&other == this)
				return *this;

			IGNORE(SetCapacity(0));

			_allocator = other._allocator;
			_lengthAndFlags = std::exchange(other._lengthAndFlags, 0);
			_storage = other._storage;
			other._storage.Small[0] = '\0';

			return *this;
		}

		constexpr operator StringView() const noexcept
		{
			return AsView();
		}

		[[nodiscard]] constexpr FORCE_INLINE size_t GetLength() const noexcept
		{
			return _lengthAndFlags & ~LARGE_FLAG;
		}

		[[nodiscard]] constexpr FORCE_INLINE size_t GetCapacity() const noexcept
		{
			return (IsSmall() ? SMALL_CAPACITY : _storage.Large.GetLength()) - 1;
		}

		[[nodiscard]] constexpr Span<char> AsSpan() noexcept
		{
			return GetStorage().Slice(0, GetLength());
		}

		[[nodiscard]] constexpr Span<const char> AsSpan() const noexcept
		{
			return GetStorage().Slice(0, GetLength());
		}

		[[nodiscard]] constexpr StringView AsView() const noexcept
		{
			return { GetStorage().AsPointer(), GetLength() };
		}

		[[nodiscard]] constexpr const char* AsCString() const noexcept
		{
			return GetStorage().AsPointer();
		}

		[[nodiscard]] Result<void, Error> SetLength(size_t newLength);
		[[nodiscard]] Result<void, Error> SetCapacity(size_t newCapacity);
		[[nodiscard]] Result<void, Error> EnsureCapacity(size_t requiredCapacity);

		[[nodiscard]] static FORCE_INLINE Result<String, Error> Create(Memory::Allocator& allocator, const StringView view)
		{
			String str(allocator);
			TRY(str.SetLength(view.GetLength()));
			view.CopyTo(str.GetStorage());
			return Ok(std::move(str));
		}

		[[nodiscard]] static FORCE_INLINE Result<String, Error> Create(Memory::Allocator& allocator, const String& other)
		{
			return Create(allocator, other.AsView());
		}

		[[nodiscard]] static FORCE_INLINE Result<String, Error> Create(Memory::Allocator& allocator, const char* cstr)
		{
			return Create(allocator, StringView(cstr));
		}

		template<size_t length>
		[[nodiscard]] static FORCE_INLINE Result<String, Error> Create(Memory::Allocator& allocator, const char (&cstr)[length])
		{
			return Create(allocator, StringView(cstr, length - 1));
		}

		template<size_t length>
		[[nodiscard]] static FORCE_INLINE Result<String, Error> Create(Memory::Allocator& allocator, const char ch, const size_t count)
		{
			String str(allocator);
			TRY(str.SetLength(count));

			const auto ptr = str.GetStorage().AsPointer();
			for (size_t i = 0; i < count; i++)
				ptr[i] = ch;

			return Ok(std::move(str));
		}

		[[nodiscard]] Result<void, Error> Assign(StringView view);

		[[nodiscard]] FORCE_INLINE Result<void, Error> Assign(const String& other)
		{
			return Assign(other.AsView());
		}

		[[nodiscard]] FORCE_INLINE Result<void, Error> Assign(const char* cstr)
		{
			return Assign(StringView(cstr));
		}

		template<size_t length>
		[[nodiscard]] FORCE_INLINE Result<void, Error> Assign(const char (&cstr)[length])
		{
			return Assign(StringView(cstr, length - 1));
		}

		[[nodiscard]] FORCE_INLINE Result<void, Error> Assign(const char ch)
		{
			return Assign(StringView(&ch, 1));
		}

		[[nodiscard]] Result<void, Error> Append(StringView view);

		[[nodiscard]] FORCE_INLINE Result<void, Error> Append(const String& other)
		{
			return Append(other.AsView());
		}

		[[nodiscard]] FORCE_INLINE Result<void, Error> Append(const char* cstr)
		{
			return Append(StringView(cstr));
		}

		template<size_t length>
		[[nodiscard]] FORCE_INLINE Result<void, Error> Append(const char (&cstr)[length])
		{
			return Append(StringView(cstr, length - 1));
		}

		[[nodiscard]] FORCE_INLINE Result<void, Error> Append(const char ch)
		{
			return Append(StringView(&ch, 1));
		}

		template<typename... Args>
		[[nodiscard]] static Result<String, Error> Format(Memory::Allocator& allocator, StringView format, Args&&... args);

		[[nodiscard]] static Result<String, Error> Concat(Memory::Allocator& allocator, const StringView left, const StringView right)
		{
			String str(allocator);
			TRY(str.SetLength(left.GetLength() + right.GetLength()));
			const auto span = str.AsSpan();
			left.CopyTo(span);
			right.CopyTo(span.Slice(left.GetLength()));
			return Ok(std::move(str));
		}

		template<Hashing::HashAlgorithm HashAlgo>
		friend void ProcessHash(HashAlgo& algorithm, const String& str) noexcept
		{
			algorithm.ProcessBytes(str.AsView());
		}

		constexpr friend std::strong_ordering operator<=>(const StringView& a, const String& b) noexcept
		{
			return a <=> b.AsView();
		}

		constexpr friend std::strong_ordering operator<=>(const String& a, const StringView& b) noexcept
		{
			return a.AsView() <=> b;
		}

		constexpr friend bool operator==(const String& a, const StringView& b) noexcept
		{
			return std::is_eq(a <=> b);
		}

		constexpr friend bool operator!=(const String& a, const StringView& b) noexcept
		{
			return std::is_neq(a <=> b);
		}

	private:
		[[nodiscard]] constexpr FORCE_INLINE Span<char> GetStorage() noexcept
		{
			return IsSmall() ? Span(_storage.Small, SMALL_CAPACITY) : _storage.Large;
		}

		[[nodiscard]] constexpr FORCE_INLINE Span<const char> GetStorage() const noexcept
		{
			return IsSmall() ? Span(_storage.Small, SMALL_CAPACITY) : _storage.Large;
		}

		[[nodiscard]] constexpr FORCE_INLINE bool IsSmall() const noexcept
		{
			return !(_lengthAndFlags & LARGE_FLAG);
		}

		Memory::Allocator* _allocator;
		size_t _lengthAndFlags { 0 };

		union
		{
			Span<char> Large;
			char Small[SMALL_CAPACITY];
		} _storage;
	};
}

#include "String.tpp"
