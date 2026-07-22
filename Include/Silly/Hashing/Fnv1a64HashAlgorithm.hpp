#pragma once
#include <cstdint>

#include "HashAlgorithm.hpp"
#include "../Span.hpp"

namespace Silly::Hashing
{
	class Fnv1a64HashAlgorithm final : public HashAlgorithmBase
	{
		static constexpr uint64_t _prime = 0x100000001B3;

	public:
		using HashType = uint64_t;

		explicit Fnv1a64HashAlgorithm(const uint64_t seed = 0xCBF29CE484222325) : _seed(seed)
		{
			Reset();
		}

		void Reset(const uint64_t seed) noexcept
		{
			_seed = seed;
			Reset();
		}

		[[nodiscard]] static size_t GetHashSize() noexcept
		{
			return 8;
		}

		void Reset() noexcept
		{
			_hash = _seed;
		}

		void ProcessBytes(const Span<const std::byte> data) noexcept
		{
			for (const auto b : data)
				_hash = (_hash ^ static_cast<uint64_t>(b)) * _prime;
		}

		void Finalize(const Span<std::byte> hash) noexcept
		{
			if (hash.GetLength() < GetHashSize())
				return;

			auto h = _hash;
			if constexpr (std::endian::native == std::endian::big)
				h = std::byteswap(h);

			Span(&h, 1).AsBytes().CopyTo(hash);
		}

	private:
		uint64_t _seed;
		uint64_t _hash { 0 };
	};

	static_assert(IntHashAlgorithm<Fnv1a64HashAlgorithm>);
}
