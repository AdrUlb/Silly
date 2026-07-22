#pragma once
#include "Empty.hpp"

namespace Silly
{
	template<typename T>
	class ReferenceWrapper
	{
	public:
		using Type = T;

		explicit constexpr ReferenceWrapper(T& ref) noexcept : _ptr(__builtin_addressof(ref)) {}

		constexpr ReferenceWrapper(const ReferenceWrapper&) noexcept = default;
		constexpr ReferenceWrapper& operator=(const ReferenceWrapper&) noexcept = default;

		ReferenceWrapper(T&&) = delete;
		ReferenceWrapper& operator=(T&&) = delete;

		constexpr T& Get() const noexcept { return *_ptr; }
		constexpr operator T&() const noexcept { return *_ptr; }

	private:
		T* _ptr;
	};

	template<typename T> struct WrappedRef
	{
		using Type = T;
	};

	template<typename T> struct UnwrappedRef
	{
		using Type = T;
	};

	template<typename T> struct WrappedRef<T&>
	{
		using Type = ReferenceWrapper<T>;
	};

	template<typename T> struct UnwrappedRef<ReferenceWrapper<T>>
	{
		using Type = T;
	};

	// Deliberately not "unwrapped"
	template<> struct WrappedRef<void>
	{
		using Type = Empty;
	};

	template<typename T, typename WrappedT> constexpr decltype(auto) UnwrapRef(WrappedT&& value)
	{
		if constexpr (std::is_reference_v<T>)
			return value.Get();
		else
			return std::forward<WrappedT>(value);
	}
}
