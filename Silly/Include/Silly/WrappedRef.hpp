#pragma once
#include <functional>

#include "Empty.hpp"

namespace Silly
{
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
		using Type = std::reference_wrapper<T>;
	};

	template<typename T> struct UnwrappedRef<std::reference_wrapper<T>>
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
			return value.get();
		else
			return std::forward<WrappedT>(value);
	}
}
