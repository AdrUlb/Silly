#pragma once
#include "Extern.hpp"
#include "Option.hpp"

namespace Silly
{
	template<typename T> struct OkValue
	{
		T&& Value;
	};

	template<typename T> struct ErrValue
	{
		T&& Value;
	};

	template<typename... Args> struct OkArgs
	{
		std::tuple<Args&&...> Value;
	};

	template<typename... Args> struct ErrArgs
	{
		std::tuple<Args&&...> Value;
	};

	// Forward the given argument, wrapped in the OkValue<T> proxy type
	template<typename T> [[nodiscard]] constexpr auto Ok(T&& value)
	{
		return OkValue<T> { std::forward<T>(value) };
	}

	// Forward the given arguments, wrapped in the OkArgs<Args...> proxy type
	template<typename... Args> [[nodiscard]] constexpr auto Ok(std::in_place_t, Args&&... args)
	{
		return OkArgs<Args...> { std::forward_as_tuple(std::forward<Args>(args)...) };
	}

	// No arguments given, construct an empty OkValue
	[[nodiscard]] constexpr auto Ok()
	{
		return OkValue<Empty> { { } };
	}

	// Forward the given argument, wrapped in the ErrValue<T> proxy type
	template<typename T> [[nodiscard]] constexpr auto Err(T&& value)
	{
		return ErrValue<T> { std::forward<T>(value) };
	}

	// Forward the given arguments, wrapped in the ErrArgs<Args...> proxy type
	template<typename... Args> [[nodiscard]] constexpr auto Err(std::in_place_t, Args&&... args)
	{
		return ErrArgs<Args...> { std::forward_as_tuple(std::forward<Args>(args)...) };
	}

	// No arguments given, construct an empty ErrValue
	[[nodiscard]] constexpr auto Err()
	{
		return ErrValue<Empty> { { } };
	}

	template<typename T, typename E> class Result
	{
		template<typename> friend class Option;

		using StorageT = WrappedRef<T>::Type;
		using StorageE = WrappedRef<E>::Type;

	public:
		// Construct (copy convert) from Result<U, F>
		template<typename U, typename F>
		explicit(!std::is_convertible_v<const U&, StorageT> || !std::is_convertible_v<const F&, StorageE>) constexpr Result(const Result<U, F>& other)
			requires(std::is_constructible_v<StorageT, const U&> && std::is_constructible_v<StorageE, const F&>)
		{
			if (other.IsOk())
				_storage.template emplace<0>(std::get<0>(other._storage));
			else
				_storage.template emplace<1>(std::get<1>(other._storage));
		}

		// Construct (move convert) from Result<U, F>
		template<typename U, typename F>
		explicit(!std::is_convertible_v<U, StorageT> || !std::is_convertible_v<F, StorageE>) constexpr Result(Result<U, F>&& other)
			requires(std::is_constructible_v<StorageT, U&&> && std::is_constructible_v<StorageE, F&&>)
		{
			if (other.IsOk())
				_storage.template emplace<0>(std::get<0>(std::move(other._storage)));
			else
				_storage.template emplace<1>(std::get<1>(std::move(other._storage)));
		}

		// Construct Value from OkValue<U>
		template<typename U>
		constexpr Result(OkValue<U>&& some)
			requires(std::is_constructible_v<StorageT, U&&>) : _storage(std::in_place_index<0>, std::forward<U>(some.Value)) {}

		// Construct Value in-place from OkArgs<Args...>
		template<typename... Args>
		constexpr Result(OkArgs<Args...>&& some)
			requires(std::is_constructible_v<StorageT, Args...>) : _storage(std::in_place_index<0>,
			                                                                std::make_from_tuple<StorageT>(std::forward<decltype(some.Value)>(some.Value))) {}

		// Construct Error from ErrValue<U>
		template<typename F>
		constexpr Result(ErrValue<F>&& some)
			requires(std::is_constructible_v<StorageE, F&&>) : _storage(std::in_place_index<1>, std::forward<F>(some.Value)) {}

		// Construct Error in-place from ErrArgs<Args...>
		template<typename... Args>
		constexpr Result(ErrArgs<Args...>&& some)
			requires(std::is_constructible_v<StorageE, Args...>) : _storage(std::in_place_index<1>,
			                                                                std::make_from_tuple<StorageE>(std::forward<decltype(some.Value)>(some.Value))) {}

		constexpr Result(OkValue<Empty>&&)
			requires(std::is_void_v<T>) : _storage(std::in_place_index<0>) {}

		constexpr Result(ErrValue<Empty>&&)
			requires(std::is_void_v<E>) : _storage(std::in_place_index<1>) {}

		[[nodiscard]] constexpr FORCE_INLINE operator bool() const
		{
			return IsOk();
		}

		[[nodiscard]] constexpr FORCE_INLINE bool IsOk() const
		{
			return _storage.index() == 0;
		}

		[[nodiscard]] constexpr FORCE_INLINE bool IsErr() const
		{
			return _storage.index() == 1;
		}

		// --- UnwrapOk/UnwrapErr, references declared as UnwrappedRef<StorageT>& because void& is illegal

		// We're an l-value, we can just return a reference
		[[nodiscard]] FORCE_INLINE constexpr UnwrappedRef<StorageT>::Type& UnwrapOk() &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsOkOrCatchFire();
			return UnwrapRef<T>(std::get<0>(_storage));
		}

		// We're a const l-value we can return a const reference
		[[nodiscard]] FORCE_INLINE constexpr const UnwrappedRef<StorageT>::Type& UnwrapOk() const &
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsOkOrCatchFire();
			return UnwrapRef<T>(std::get<0>(_storage));
		}

		// We're an r-value, we cannot return a reference, we must move the actual value out
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOk() &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		{
			IsOkOrCatchFire();
			return UnwrapRef<T>(std::get<0>(std::move(_storage)));
		}

		// We're a const r-value, we cannot return a reference (we die right after returning it), nor can we move the value out (const)
		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOk() const &&
			requires(!std::is_reference_v<T> && !std::is_void_v<T>)
		= delete;

		// We're an l-value, we can just return a reference
		[[nodiscard]] FORCE_INLINE constexpr UnwrappedRef<StorageE>::Type& UnwrapErr() &
			requires(!std::is_reference_v<E> && !std::is_void_v<E>)
		{
			IsErrOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		// We're a const l-value we can return a const reference
		[[nodiscard]] FORCE_INLINE constexpr const UnwrappedRef<StorageE>::Type& UnwrapErr() const &
			requires(!std::is_reference_v<E> && !std::is_void_v<E>)
		{
			IsErrOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		// We're an r-value, we cannot return a reference, we must move the actual value out
		[[nodiscard]] FORCE_INLINE constexpr E UnwrapErr() &&
			requires(!std::is_reference_v<E> && !std::is_void_v<E>)
		{
			IsErrOrCatchFire();
			return UnwrapRef<T>(std::get<1>(std::move(_storage)));
		}

		// We're a const r-value, we cannot return a reference, nor can we move the value out
		[[nodiscard]] FORCE_INLINE constexpr E UnwrapErr() const &&
			requires(!std::is_reference_v<E> && !std::is_void_v<E>)
		= delete;

		// --- Special reference versions of UnwrapOk, UnwrapErr

		[[nodiscard]] FORCE_INLINE constexpr T UnwrapOk()
			requires(std::is_reference_v<T>)
		{
			IsOkOrCatchFire();
			return UnwrapRef<T>(std::get<0>(_storage));
		}

		[[nodiscard]] FORCE_INLINE constexpr E UnwrapErr()
			requires(std::is_reference_v<E>)
		{
			IsErrOrCatchFire();
			return UnwrapRef<T>(std::get<1>(_storage));
		}

		// --- Special void versions of UnwrapOk, UnwrapErr

		FORCE_INLINE constexpr void UnwrapOk() const
			requires(std::is_void_v<T>)
		{
			IsOkOrCatchFire();
		}

		FORCE_INLINE constexpr void UnwrapErr() const
			requires(std::is_void_v<E>)
		{
			IsErrOrCatchFire();
		}

		template<typename CallbackOk, typename CallbackErr> FORCE_INLINE constexpr auto Match(CallbackOk&& callbackOk, CallbackErr callbackErr) &
		{
			if constexpr (std::is_void_v<T> && std::is_void_v<E>)
				return IsOk() ? callbackOk() : callbackErr();
			else if constexpr (std::is_void_v<T>)
				return IsOk() ? callbackOk() : callbackErr(UnwrapRef<E>(std::get<1>(_storage)));
			else if constexpr (std::is_void_v<E>)
				return IsOk() ? callbackOk(UnwrapRef<T>(std::get<0>(_storage))) : callbackErr();
			else
				return IsOk() ? callbackOk(UnwrapRef<T>(std::get<0>(_storage))) : callbackErr(UnwrapRef<E>(std::get<1>(_storage)));
		}

		template<typename CallbackOk, typename CallbackErr> FORCE_INLINE constexpr auto Match(CallbackOk&& callbackOk, CallbackErr callbackErr) &&
		{
			if constexpr (std::is_void_v<T> && std::is_void_v<E>)
				return IsOk() ? callbackOk() : callbackErr();
			else if constexpr (std::is_void_v<T>)
				return IsOk() ? callbackOk() : callbackErr(UnwrapRef<E>(std::get<1>(std::move(_storage))));
			else if constexpr (std::is_void_v<E>)
				return IsOk() ? callbackOk(UnwrapRef<T>(std::get<0>(std::move(_storage)))) : callbackErr();
			else
				return IsOk() ? callbackOk(UnwrapRef<T>(std::get<0>(std::move(_storage)))) : callbackErr(UnwrapRef<E>(std::get<1>(std::move(_storage))));
		}

	private:
		constexpr void IsOkOrCatchFire() const
		{
			VERIFY(IsOk());
		}

		constexpr void IsErrOrCatchFire() const
		{
			VERIFY(IsErr());
		}

		std::variant<StorageT, StorageE> _storage;
	};
} // namespace Silly

#define TRY(...) \
	({ \
		auto&& result = (__VA_ARGS__); \
		if (result.IsErr()) \
			[[unlikely]] return Err(result.UnwrapErr()); \
		std::forward<decltype(result)>(result).UnwrapOk(); \
	})

#if SILLY_GLOBAL
using namespace Silly;
#endif
