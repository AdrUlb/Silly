#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace Silly
{
	// Cannot include Span here, it would cause a cycle
	template<typename> class Span;
}

namespace Silly::Hashing
{
	// Every HashAlgorithm must inherit from HashAlgorithmBase
	// The incomplete type HashAlgorithmArchetype allows concepts to check against an archetypical HashAlgorithm without providing a specific implementation
	struct HashAlgorithmBase;
	struct HashAlgorithmArchetype;

	// This defines what truly makes a HashAlgorithm
	template<typename T>
	concept HashAlgorithm =
		std::is_base_of_v<HashAlgorithmBase, T> &&
		requires(T algo, const Span<const std::byte>& bytes, const Span<std::byte>& hash)
		{
			typename T::HashType;

			{ algo.GetHashSize() } noexcept -> std::same_as<size_t>;
			{ algo.Reset() } noexcept -> std::same_as<void>;
			{ algo.ProcessBytes(bytes) } noexcept -> std::same_as<void>;
			{ algo.Finalize(hash) } noexcept -> std::same_as<void>;
		};

	// An IntHashAlgorithm is any hash algorithm that returns an integral hash, making it usable in hash maps and the likes
	template<typename Algo>
	concept IntHashAlgorithm = HashAlgorithm<Algo> && std::is_integral_v<typename Algo::HashType>;

	// Any integer is hashable, any non-integer implementing a proper ProcessHash hidden friend is also hashable
	template<typename T>
	concept Hashable =
		std::is_integral_v<T> ||
		requires(HashAlgorithmArchetype& algo, const T& value)
		{
			{ ProcessHash(algo, value) } -> std::same_as<void>;
		};

	// Convenience methods for all hash algorithms
	struct HashAlgorithmBase
	{
		// Convenience method: ProcessHash(algorithm, data) => algorithm.Process(data)
		template<HashAlgorithm Self, typename T> requires(Hashable<T>)
		void Process(this Self& self, const T& data)
		{
			/*
			 * Depends on the following hidden friend being defined in T:
			 * template<Hashing::HashAlgorithm HashAlgo> friend void ProcessHash(Self& algorithm, const T& view)
			 */
			ProcessHash(self, data);
		}

		// Convenience method: ProcessHash(algorithm, data) => algorithm.Process(data)
		// For integral types, where no ProcessHash method is implemented
		template<HashAlgorithm Self, typename T> requires(Hashable<T> && std::is_integral_v<T>)
		void Process(this Self& self, const T& value)
		{
			auto bytes = Span(&value, 1).AsBytes();

			if constexpr (std::endian::native == std::endian::big)
			{
				std::byte swapped[sizeof(T)];
				bytes.ReverseTo(swapped);
				return self.ProcessBytes(swapped);
			}
			else
				return self.ProcessBytes(bytes);
		}

		// Reset the algorithm, process the given data, and compute the final hash
		template<typename SelfRef, typename T>
		void GenerateHash(this SelfRef&& self, T&& data, const Span<std::byte>& hash) requires(Hashable<std::remove_reference_t<T>>)
		{
			using Self = std::remove_reference_t<SelfRef>;
			static_assert(HashAlgorithm<Self>);

			self.Reset();
			self.Process(std::forward<T>(data));
			self.Finalize(hash);
		}

		// Reset the algorithm, process the given data, and compute the final hash, return it by value
		// For integral hash types
		template<typename SelfRef, typename T>
		auto GenerateHash(this SelfRef&& self, T&& data) requires(Hashable<std::remove_reference_t<T>>)
		{
			using Self = std::remove_reference_t<SelfRef>;
			static_assert(IntHashAlgorithm<Self>);

			typename Self::HashType hash;
			self.Reset();
			self.Process(std::forward<T>(data));
			self.Finalize(Span { &hash, 1 }.AsBytes());

			if constexpr (std::endian::native == std::endian::big)
				hash = std::byteswap(hash);

			return hash;
		}
	};

	// The HashAlgorithm archetype, at minimum every HashAlgorithm must implement this interface
	struct HashAlgorithmArchetype : HashAlgorithmBase
	{
		using HashType = std::common_type<>;
		size_t GetHashSize() noexcept;
		void Reset() noexcept;
		void ProcessBytes(const Span<const std::byte>& bytes) noexcept;
		void Finalize(const Span<std::byte>& hash) noexcept;
	};
} // namespace Silly

// Finally, include span down here so the implementation is found when the template method bodies are evaluated
#include "Silly/Span.hpp"
