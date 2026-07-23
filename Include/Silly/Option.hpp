#pragma once
#include <memory>
#include <type_traits>
#include <variant>

#include "Empty.hpp"
#include "Macros.hpp"
#include "WrappedRef.hpp"

namespace Silly
{
	enum NoneTag {};

	inline constexpr NoneTag None { };

	template<typename T> struct SomeValue
	{
		T&& Value;
	};

	template<typename... Args> struct SomeArgs
	{
		std::tuple<Args&&...> Value;
	};

	// Forward the given argument, wrapped in the SomeValue<T> proxy type
	template<typename T> [[nodiscard]] constexpr auto Some(T&& value)
	{
		return SomeValue<T> { std::forward<T>(value) };
	}

	// Forward the given arguments, wrapped in the SomeArgs<Args...> proxy type
	template<typename... Args> [[nodiscard]] constexpr auto Some(std::in_place_t, Args&&... args)
	{
		return SomeArgs<Args...> { std::forward_as_tuple(std::forward<Args>(args)...) };
	}

	// No arguments given, construct an empty SomeValue
	[[nodiscard]] constexpr auto Some()
	{
		return SomeValue<Empty> { { } };
	}

	template<typename T> class [[nodiscard]] Option
	{
		template<typename> friend class Option;

		using StorageT = WrappedRef<T>::Type;

	public:
		// Construct without a value
		constexpr Option(NoneTag) noexcept : _storage(std::in_place_index<0>) {}

		// Construct (copy convert) from Option<U>
		template<typename U>
		explicit(!std::is_convertible_v<const U&, StorageT>) constexpr Option(const Option<U>& other)
			requires(std::is_constructible_v<StorageT, const U&>)
		{
			if (other.HasValue())
				_storage.template emplace<1>(std::get<1>(other._storage));
		}

		// Construct (move convert) from Option<U>
		template<typename U>
		explicit(!std::is_convertible_v<U, StorageT>) constexpr Option(Option<U>&& other)
			requires(std::is_constructible_v<StorageT, U&&>)
		{
			if (other.HasValue())
				_storage.template emplace<1>(std::get<1>(std::move(other._storage)));
		}

		// Construct from SomeValue<U>
		template<typename U>
		constexpr Option(SomeValue<U>&& some)
			requires(std::is_constructible_v<StorageT, U&&>) : _storage(std::in_place_index<1>, std::forward<U>(some.Value)) {}

		// Construct in-place from SomeArgs<Args...>
		template<typename... Args>
		constexpr Option(SomeArgs<Args...>&& some)
			requires(std::is_constructible_v<StorageT, Args...>)
			: _storage(std::in_place_index<1>, std::make_from_tuple<StorageT>(std::forward<decltype(some.Value)>(some.Value))) {}

		constexpr Option(SomeValue<Empty>&&)
			requires(std::is_void_v<T>) : _storage(std::in_place_index<1>) {}

		[[nodiscard]] constexpr decltype(auto) operator*() const &
			requires(!std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		[[nodiscard]] constexpr decltype(auto) operator*() &&
			requires(!std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRef<T>(std::get<1>(std::move(_storage)));
		}

		[[nodiscard]] constexpr auto* operator->()
			requires(!std::is_void_v<T> && !std::is_reference_v<T>)
		{
			IsValueOrCatchFire();
			return &std::get<1>(_storage);
		}

		[[nodiscard]] constexpr const auto* operator->() const
			requires(!std::is_void_v<T> && !std::is_reference_v<T>)
		{
			IsValueOrCatchFire();
			return &std::get<1>(_storage);
		}

		[[nodiscard]] constexpr auto* operator->()
			requires(std::is_reference_v<T>)
		{
			IsValueOrCatchFire();
			return &std::get<1>(_storage).Get();
		}

		[[nodiscard]] constexpr FORCE_INLINE operator bool() const
		{
			return HasValue();
		}

		[[nodiscard]] constexpr FORCE_INLINE bool HasValue() const
		{
			return _storage.index() == 1;
		}

		// --- Unwrap/UnwrapOr/UnwrapOrLazy, references declared as UnwrappedRef<StorageT>& because void& is illegal

		// We're an l-value, we can just return a reference
		[[nodiscard]] FORCE_INLINE constexpr UnwrappedRef<StorageT>::Type& Unwrap() &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		// We're a const l-value we can return a const reference
		[[nodiscard]] FORCE_INLINE constexpr const UnwrappedRef<StorageT>::Type& Unwrap() const &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		// We're an r-value, we cannot return a reference, we must move the actual value out
		[[nodiscard]] FORCE_INLINE constexpr T Unwrap() &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRelease();
		}

		// We're a const r-value, we cannot return a reference, nor can we move the value out
		[[nodiscard]] FORCE_INLINE constexpr T Unwrap() const &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		= delete;

		// We're a (const) l-value we can return the value
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOr(UnwrappedRef<StorageT>::Type fallback) const &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			if (HasValue())
				return UnwrapRef<T>(std::get<1>(_storage));

			return fallback;
		}

		// We're an r-value we can move the value out
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOr(UnwrappedRef<StorageT>::Type fallback) &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			if (HasValue())
				return UnwrapRelease();

			return fallback;
		}

		template<typename Callback>
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOrLazy(Callback&& callback) const &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			if (HasValue())
				return UnwrapRef<T>(std::get<1>(_storage));

			return std::forward<Callback>(callback)();
		}

		template<typename Callback>
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOrLazy(Callback&& callback) &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			if (HasValue())
				return UnwrapRelease();

			return std::forward<Callback>(callback)();
		}

		// --- Special reference versions of Unwrap, UnwrapOr, UnwrapOrLazy

		// Just return a plain reference if T is a reference
		[[nodiscard]] FORCE_INLINE constexpr T Unwrap()
			requires(std::is_reference_v<T>)
		{
			IsValueOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOr(UnwrappedRef<StorageT>::Type fallback) &&
			requires(std::is_reference_v<T>)
		{
			if (HasValue())
				return UnwrapRef<T>(std::get<1>(_storage));

			return fallback;
		}

		template<typename Callback>
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOrLazy(Callback&& callback)
			requires(std::is_reference_v<T>)
		{
			if (HasValue())
				return UnwrapRef<T>(std::get<1>(_storage));

			return std::forward<Callback>(callback)();
		}

		// --- Special void versions of Unwrap, UnwrapOr, UnwrapOrLazy

		FORCE_INLINE constexpr void Unwrap() const
			requires(std::is_void_v<T>)
		{
			IsValueOrCatchFire();
		}

		FORCE_INLINE constexpr void UnwrapOr() const
			requires(std::is_void_v<T>)
		{
			IsValueOrCatchFire();
		}

		template<typename Callback>
		FORCE_INLINE constexpr void UnwrapOrLazy(Callback&& callback)
			requires(std::is_void_v<T>)
		{
			if (!HasValue())
				return callback();
		}

	private:
		constexpr void IsValueOrCatchFire() const
		{
			VERIFY(HasValue());
		}

		// Move the value out
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapRelease()
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsValueOrCatchFire();
			auto value = UnwrapRef<T>(std::get<1>(std::move(_storage)));
			return value;
		}

		std::variant<Empty, StorageT> _storage;
	};
} // namespace Silly
