#pragma once
#include <type_traits>
#include <utility>

namespace Silly
{
	template<typename TKey, typename TValue>
	struct KeyValuePair
	{
		TKey Key;
		TValue Value;

		template<size_t index>
		auto& get() &
		{
			static_assert(index <= 1);

			if constexpr (index == 0)
				return static_cast<const TKey&>(Key);
			if constexpr (index == 1)
				return static_cast<TValue&>(Value);
		}

		template<size_t index>
		auto& get() const &
		{
			static_assert(index <= 1);

			if constexpr (index == 0)
				return static_cast<const TKey&>(Key);
			if constexpr (index == 1)
				return static_cast<const TValue&>(Value);
		}
	};
}

template<typename TKey, typename TValue>
struct std::tuple_size<Silly::KeyValuePair<TKey, TValue>> : std::integral_constant<size_t, 2> {};

template<typename TKey, typename TValue>
struct std::tuple_element<0, Silly::KeyValuePair<TKey, TValue>>
{
	using type = const TKey;
};

template<typename TKey, typename TValue>
struct std::tuple_element<1, Silly::KeyValuePair<TKey, TValue>>
{
	using type = TValue;
};
