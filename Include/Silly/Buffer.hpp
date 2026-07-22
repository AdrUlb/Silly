#pragma once
#include "Memory/Allocator.hpp"
#include "Error.hpp"
#include "Result.hpp"

namespace Silly
{
	template<typename T> class Buffer
	{
	public:
		using iterator = Span<T>::iterator;

		[[nodiscard]] FORCE_INLINE iterator begin()
		{
			return _span.begin();
		}

		[[nodiscard]] FORCE_INLINE iterator end()
		{
			return _span.end();
		}

		[[nodiscard]] static Result<Buffer, Error> Create(Memory::Allocator& allocator, const size_t length)
		{
			const auto span = TRY(allocator.AllocateSpan<T>(length));
			return Ok(Buffer(allocator, span));
		}

		[[nodiscard]] Result<void, Error> Resize(size_t newLength)
			requires(std::is_trivially_copyable_v<T>)
		{
			_span = TRY(_allocator->ReallocateSpan(_span, newLength));
			return Ok();
		}

		[[nodiscard]] Span<T> FORCE_INLINE AsSpan() const
		{
			return _span;
		}

		[[nodiscard]] size_t FORCE_INLINE GetLength() const
		{
			return _span.GetLength();
		}

		[[nodiscard]] FORCE_INLINE bool IsEmpty() const
		{
			return _span.IsEmpty();
		}

		[[nodiscard]] T& operator[](size_t index) const
		{
			return _span[index];
		}

	private:
		explicit Buffer(Memory::Allocator& allocator, Span<T> span) : _allocator(&allocator), _span(span) {}

		Memory::Allocator* _allocator;
		Span<T> _span;
	};
} // namespace Silly
