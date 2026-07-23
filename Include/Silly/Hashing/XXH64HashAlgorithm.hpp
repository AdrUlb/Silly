#pragma once
#include <bit>
#include <cstdint>
#include <cstring>

#include "HashAlgorithm.hpp"
#include "Silly//Macros.hpp"

namespace Silly::Hashing
{
	/*
	 * SPDX-License-Identifier: BSD-2-Clause
	 * The XXH64 algorithm was adapted from xxHash Library source code
	 * Reference implementation is Copyright (c) 2012-2021 Yann Collet
	 * This implementation is Copyright (C) 2026 Adrian Ulbrich
	 * See ATTRIBUTIONS.TXT for the full license text
	 */
	class XXH64HashAlgorithm final : public HashAlgorithmBase
	{
		static constexpr uint64_t _prime1 = 0x9E3779B185EBCA87ULL;
		static constexpr uint64_t _prime2 = 0xC2B2AE3D27D4EB4FULL;
		static constexpr uint64_t _prime3 = 0x165667B19E3779F9ULL;
		static constexpr uint64_t _prime4 = 0x85EBCA77C2B2AE63ULL;
		static constexpr uint64_t _prime5 = 0x27D4EB2F165667C5ULL;

	public:
		using HashType = uint64_t;

		explicit XXH64HashAlgorithm(const uint64_t seed = 0) : _seed(seed)
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

		// XXH64_reset
		void Reset() noexcept
		{
			_v1 = _seed + _prime1 + _prime2;
			_v2 = _seed + _prime2;
			_v3 = _seed;
			_v4 = _seed - _prime1;
			_tempCount = 0;
			_totalCount = 0;
		}

		// XXH64_update
		void ProcessBytes(const Span<const std::byte> data) noexcept
		{
			auto ptr = data.AsPointer();
			auto len = data.GetLength();

			_totalCount += len;

			if (_tempCount > 0)
			{
				const auto missing = 32 - _tempCount;
				if (len < missing)
				{
					data.CopyTo(Span { _temp + _tempCount, len });
					_tempCount += len;
					return;
				}

				data.Slice(0, missing).CopyTo(Span { _temp + _tempCount, missing });
				Process4(_temp);
				ptr += missing;
				len -= missing;
				_tempCount = 0;
			}

			while (len >= 32)
			{
				Process4(ptr);
				ptr += 32;
				len -= 32;
			}

			if (len > 0)
			{
				Span { ptr, len }.CopyTo(Span { _temp, len });
				_tempCount = len;
			}
		}

		// XXH64_digest
		void Finalize(const Span<std::byte> hash) noexcept
		{
			uint64_t h;
			if (_totalCount >= 32)
			{
				h = std::rotl(_v1, 1) + std::rotl(_v2, 7) + std::rotl(_v3, 12) + std::rotl(_v4, 18);

				h = MergeLane(h, _v1);
				h = MergeLane(h, _v2);
				h = MergeLane(h, _v3);
				h = MergeLane(h, _v4);
			}
			else
			{
				h = _seed + _prime5;
			}

			h += _totalCount;

			auto ptr = _temp;
			const auto end = _temp + _tempCount;

			while (ptr + 8 <= end)
			{
				const uint64_t k1 = Round(0, Read64(ptr));
				h ^= k1;
				h = std::rotl(h, 27) * _prime1 + _prime4;
				ptr += 8;
			}

			if (ptr + 4 <= end)
			{
				h ^= Read32(ptr) * _prime1;
				h = std::rotl(h, 23) * _prime2 + _prime3;
				ptr += 4;
			}

			while (ptr < end)
			{
				h ^= static_cast<uint64_t>(*ptr) * _prime5;
				h = std::rotl(h, 11) * _prime1;
				ptr++;
			}

			_tempCount = 0;

			h = Avalanche(h);

			if (hash.GetLength() < GetHashSize())
				return;

			if constexpr (std::endian::native == std::endian::big)
				h = std::byteswap(h);

			Span(&h, 1).AsBytes().CopyTo(hash);
		}

	private:
		FORCE_INLINE void Process4(const std::byte* ptr)
		{
			const auto k1 = Read64(ptr);
			ptr += 8;
			const auto k2 = Read64(ptr);
			ptr += 8;
			const auto k3 = Read64(ptr);
			ptr += 8;
			const auto k4 = Read64(ptr);
			ptr += 8;

			_v1 = Round(_v1, k1);
			_v2 = Round(_v2, k2);
			_v3 = Round(_v3, k3);
			_v4 = Round(_v4, k4);
		}

		FORCE_INLINE static uint64_t Read32(const std::byte* ptr)
		{
			uint32_t result;
			memcpy(&result, ptr, sizeof(uint32_t));
			if constexpr (std::endian::native == std::endian::big)
				result = std::byteswap(result);

			return result;
		}

		FORCE_INLINE static uint64_t Read64(const std::byte* ptr)
		{
			uint64_t result;
			memcpy(&result, ptr, sizeof(uint64_t));
			if constexpr (std::endian::native == std::endian::big)
				result = std::byteswap(result);

			return result;
		}

		FORCE_INLINE static uint64_t Round(uint64_t acc, const uint64_t input)
		{
			acc += input * _prime2;
			acc = std::rotl(acc, 31);
			acc *= _prime1;
			return acc;
		}

		FORCE_INLINE static uint64_t MergeLane(const uint64_t h, uint64_t v)
		{
			v *= _prime2;
			v = std::rotl(v, 31);
			v *= _prime1;
			return (h ^ v) * _prime1 + _prime4;
		}

		FORCE_INLINE static uint64_t Avalanche(uint64_t hash)
		{
			hash ^= hash >> 33;
			hash *= _prime2;
			hash ^= hash >> 29;
			hash *= _prime3;
			hash ^= hash >> 32;
			return hash;
		}

		uint64_t _seed;
		uint64_t _v1 { }, _v2 { }, _v3 { }, _v4 { };
		std::byte _temp[32] { };
		uint64_t _tempCount { 0 };
		uint64_t _totalCount { 0 };
	};

	static_assert(IntHashAlgorithm<XXH64HashAlgorithm>);
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
