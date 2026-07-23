#pragma once
#include "KeyValuePair.hpp"
#include "Hashing/Fnv1a64HashAlgorithm.hpp"
#include "Hashing/XXH64HashAlgorithm.hpp"
#include "Memory/Allocator.hpp"

namespace Silly
{
	template<typename TKey, typename TValue, Hashing::IntHashAlgorithm HashAlgorithm = Hashing::XXH64HashAlgorithm>
	class FlatMap
	{
		using KVP = KeyValuePair<const TKey, TValue>;

		enum class DeletionStrategy
		{
			Tombstone,
			ShiftBack
		};

		using HashType = HashAlgorithm::HashType;

	private:
		template<bool> friend class Iterator;

		static constexpr size_t _initialCapacity = 16;

		// Avoid collisions by rehashing at a load of 70%
		static constexpr size_t _maxLoadFactor = 7;
		static constexpr size_t _maxLoadDivisor = 10;

		enum class SlotState : uint8_t
		{
			Empty,
			Occupied,
			Tombstone,
		};

		struct Slot
		{
			// ReSharper disable once CppPossiblyUninitializedMember
			Slot() {}

			~Slot()
			{
#if !defined(NDEBUG)
				VERIFY(!_dataValid);
#endif
			}

			SlotState GetState()
			{
				return _state;
			}

			HashType GetHash()
			{
#if !defined(NDEBUG)
				VERIFY(_dataValid);
#endif
				return _hash;
			}

			KVP& GetData()
			{
#if !defined(NDEBUG)
				VERIFY(_dataValid);
#endif
				return _data;
			}

			template<typename... Args> void EmplaceData(HashType hash, Args&&... args)
			{
#if !defined(NDEBUG)
				VERIFY(!_dataValid);
#endif
				std::construct_at(&_data, std::forward<Args>(args)...);
				_hash = hash;
				_state = SlotState::Occupied;
#if !defined(NDEBUG)
				_dataValid = true;
#endif
			}

			void ClearData(bool tombstone = false)
			{
#if !defined(NDEBUG)
				VERIFY(_dataValid);
#endif
				std::destroy_at(&_data);
				_state = tombstone ? SlotState::Tombstone : SlotState::Empty;
#if !defined(NDEBUG)
				_dataValid = false;
#endif
			}

		private:
			SlotState _state { SlotState::Empty };
			HashType _hash;

			// Wrap Data in an anonymous union for manual lifetime management
			union
			{
				KVP _data;
			};
#if !defined(NDEBUG)
			bool _dataValid { false };
#endif
		};

		template<bool isConst> struct Iterator
		{
		private:
			using Map = std::conditional_t<isConst, const FlatMap, FlatMap>;

		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = KVP;
			using pointer = std::conditional_t<isConst, const value_type*, value_type*>;
			using reference = std::conditional_t<isConst, const value_type&, value_type&>;

			Iterator(Map* map, const size_t index) : _map(map), _index(index)
			{
				SkipToNext();
			}

			reference operator*() const
			{
				return _map->_slots[_index].GetData();
			}

			pointer operator->() const
			{
				return &_map->_slots[_index].GetData();
			}

			Iterator& operator++()
			{
				_index++;
				SkipToNext();
				return *this;
			}

			Iterator operator++(int)
			{
				const auto temp = *this;
				++(*this);
				return temp;
			}

			[[nodiscard]] friend bool operator==(const Iterator& a, const Iterator& b)
			{
				return a._index == b._index;
			}

			[[nodiscard]] friend bool operator!=(const Iterator& a, const Iterator& b)
			{
				return a._index != b._index;
			}

		private:
			void SkipToNext()
			{
				while (_index < _map->GetCapacity() && _map->_slots[_index].GetState() != SlotState::Occupied)
					_index++;
			}

			Map* _map;
			size_t _index;
		};

	public:
		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		static constexpr DeletionStrategy DefaultDeletionStrategy =
			std::is_nothrow_move_constructible_v<TKey> && std::is_nothrow_move_constructible_v<TValue>
			? DeletionStrategy::ShiftBack
			: DeletionStrategy::Tombstone;

		explicit FlatMap(Memory::Allocator& allocator,
		                 const DeletionStrategy deletionStrategy = DefaultDeletionStrategy,
		                 const HashAlgorithm& hashAlgorithm = HashAlgorithm { })
			: _allocator(&allocator), _deletionStrategy(deletionStrategy), _hashAlgorithm(hashAlgorithm) {}

		~FlatMap()
		{
			Clear(true);
		}

		FlatMap(const FlatMap&) = delete;
		FlatMap& operator=(const FlatMap&) = delete;

		FlatMap(FlatMap&& other) noexcept
			: _allocator(other._allocator),
			  _deletionStrategy(other._deletionStrategy),
			  _hashAlgorithm(std::move(other._hashAlgorithm)),
			  _slots(std::exchange(other._slots, { })),
			  _count(std::exchange(other._count, 0)),
			  _tombstoneCount(std::exchange(other._tombstoneCount, 0)),
			  _resizeThreshold(std::exchange(other._resizeThreshold, 0)) {}

		FlatMap& operator=(FlatMap&& other) noexcept
		{
			if (&other == this)
				return *this;

			Clear(true);

			_allocator = other._allocator;
			_deletionStrategy = other._deletionStrategy;
			_hashAlgorithm = std::move(other._hashAlgorithm);
			_slots = std::exchange(other._slots, { });
			_count = std::exchange(other._count, 0);
			_tombstoneCount = std::exchange(other._tombstoneCount, 0);
			_resizeThreshold = std::exchange(other._resizeThreshold, 0);

			return *this;
		}

		iterator begin()
		{
			return iterator { this, 0 };
		}

		iterator end()
		{
			return iterator { this, GetCapacity() };
		}

		const_iterator begin() const
		{
			return const_iterator { this, 0 };
		}

		const_iterator end() const
		{
			return const_iterator { this, GetCapacity() };
		}

		[[nodiscard]] size_t GetCount() const
		{
			return _count;
		}

		[[nodiscard]] size_t GetCapacity() const
		{
			return _slots.GetLength();
		}

		template<typename K, typename V>
		[[nodiscard]] Result<void, Error> Insert(K&& key, V&& value)
		{
			// If we're about to exceed the max load factor, rehash
			if (_count + _tombstoneCount >= _resizeThreshold)
			{
				const auto newCapacity = GetCapacity() == 0 ? _initialCapacity : GetCapacity() * 2;
				TRY(Rehash(newCapacity));
			}

			// Find entry index for the given key
			HashType hash;
			const auto index = Probe(key, &hash);
			auto& slot = _slots[index];

			// If the slot is currently occupied, overwrite the value and return
			if (slot.GetState() == SlotState::Occupied)
			{
				slot.GetData().Value = std::forward<V>(value);
				return Ok();
			}

			// If replacing a tombstone, reduce tombstone count
			if (slot.GetState() == SlotState::Tombstone)
				_tombstoneCount--;

			// Insert the object
			slot.EmplaceData(hash, std::forward<K>(key), std::forward<V>(value));
			_count++;

			return Ok();
		}

		[[nodiscard]] Option<TValue&> Find(const TKey& key)
		{
			if (GetCapacity() == 0)
				return None;

			const size_t index = Probe(key);
			auto& slot = _slots[index];

			if (slot.GetState() != SlotState::Occupied)
				return None;

			return Some(slot.GetData().Value);
		}

		[[nodiscard]] Option<const TValue&> Find(const TKey& key) const
		{
			if (GetCapacity() == 0)
				return None;

			const size_t index = Probe(key);
			auto& slot = _slots[index];

			if (slot.GetState() != SlotState::Occupied)
				return None;

			return Some(slot.GetData().Value);
		}

		bool Remove(const TKey& key)
		{
			switch (_deletionStrategy)
			{
				case DeletionStrategy::Tombstone: return Remove_Tombstone(key);
				case DeletionStrategy::ShiftBack: return Remove_ShiftBack(key);
			}

			return false;
		}

		void Clear(const bool resetCapacity = false)
		{
			for (size_t i = 0; i < GetCapacity(); i++)
			{
				if (_slots[i].GetState() == SlotState::Occupied)
					_slots[i].ClearData();
			}

			_count = 0;
			_tombstoneCount = 0;

			if (resetCapacity && GetCapacity() > 0)
			{
				_allocator->DeallocateSpan(_slots);
				_slots = { };
				_resizeThreshold = 0;
			}
		}

	private:
		[[nodiscard]] size_t Probe(const TKey& key, HashType* outHashType = nullptr) const
		{
			VERIFY(GetCapacity() > 0); // This method may not be called if the map has no capacity
			VERIFY(_count<GetCapacity()); // The probe loop assumes that the map is never allowed to reach its full capacity => max load factor may not be 100%
			const auto hash = _hashAlgorithm.GenerateHash(key);

			// Optionally return the hash to the caller
			if (outHashType)
				*outHashType = hash;

			// Indices are generated by taking the key's hash and bounding it to the array size, wrapping around at the boundary
			const size_t mask = GetCapacity() - 1;
			size_t index = hash & mask; // index = hash % capacity, but faster (requires capacity to be a power of 2
			size_t tombstoneIndex = SIZE_MAX;

			while (_slots[index].GetState() != SlotState::Empty) // Probe for the first empty slot after the hash index
			{
				// The exact key was already present in the map! We can return its index
				if (_slots[index].GetState() == SlotState::Occupied &&
				    _slots[index].GetHash() == hash &&
				    _slots[index].GetData().Key == key)
					return index;

				// Overwriting a tombstone is better than using up an empty slot, if we see a tombstone remember where we found it
				if (_slots[index].GetState() == SlotState::Tombstone && tombstoneIndex == SIZE_MAX)
					tombstoneIndex = index;

				index = (index + 1) & mask;
			}

			// The exact key did NOT exist in the map, return the index of the first tombstone or the first entry instead
			return tombstoneIndex != SIZE_MAX ? tombstoneIndex : index;
		}

		[[nodiscard]] Result<void, Error> Rehash(const size_t newCapacity)
		{
			const auto newSlots = TRY(_allocator->AllocateSpan<Slot>(newCapacity));
			std::uninitialized_value_construct(newSlots.begin(), newSlots.end());

			if (GetCapacity() > 0)
			{
				// Insert all old items into the map again
#if defined(__EXCEPTIONS)
				try
#endif
				{
					const size_t mask = newCapacity - 1;

					for (size_t i = 0; i < GetCapacity(); i++)
					{
						auto& slot = _slots[i];
						const auto& data = slot.GetData();

						if (slot.GetState() != SlotState::Occupied)
							continue;

						size_t newI = slot.GetHash() & mask;

						while (newSlots[newI].GetState() != SlotState::Empty)
							newI = (newI + 1) & mask;

						auto& newSlot = newSlots[newI];

						newSlot.EmplaceData(slot.GetHash(), std::move_if_noexcept(data.Key), std::move_if_noexcept(data.Value));
						slot.ClearData();
					}
				}
#if defined(__EXCEPTIONS)
				catch (...) // Exceptions are enabled, catch any and restore the map's state
				{
					for (size_t i = 0; i < newCapacity; i++)
					{
						if (newSlots[i].GetState() == SlotState::Occupied)
							newSlots[i].ClearData();
					}

					_allocator->DeallocateSpan(newSlots);
					return Err(Error("An error occured while copying the objects during rehashing."));
				}
#endif
			}

			_allocator->DeallocateSpan(_slots);
			_slots = newSlots;
			_tombstoneCount = 0;
			_resizeThreshold = (GetCapacity() * _maxLoadFactor + _maxLoadDivisor - 1) / _maxLoadDivisor;

			return Ok();
		}

		bool Remove_Tombstone(const TKey& key)
		{
			if (GetCapacity() == 0)
				return false;

			const size_t index = Probe(key);
			auto& slot = _slots[index];

			if (slot.GetState() != SlotState::Occupied)
				return false;

			slot.ClearData(true);
			_count--;
			_tombstoneCount++;
			return true;
		}

		bool Remove_ShiftBack(const TKey& key)
		{
			if (GetCapacity() == 0)
				return false;

			const auto hash = _hashAlgorithm.GenerateHash(key);

			const size_t mask = GetCapacity() - 1;
			size_t i = hash & mask;

			while (true)
			{
				auto& slot = _slots[i];

				if (slot.GetState() == SlotState::Empty) // We found an empty element before our target, the requested element cannot be present
					return false;

				if (slot.GetState() == SlotState::Occupied && slot.GetHash() == hash && slot.GetData().Key == key)
				// We found out element, delete it and break out of the loop!
				{
					slot.ClearData();
					_count--;
					break;
				}

				i = (i + 1) & mask;
			}

			// We left a hole where we deleted the element
			auto holeI = i;

			i = (i + 1) & mask;

			// Shift all subsequent elements back if necessary
			while (true)
			{
				auto& slot = _slots[i];

				if (slot.GetState() == SlotState::Empty) // We're all done shifting!
					return true;

				if (slot.GetState() == SlotState::Tombstone) // Skip tombstones, in case they exist (they shouldn't)
				{
					i = (i + 1) & mask;
					continue;
				}

				// Get the index that this element would ideally want to live at assuming 0 collisions
				const auto desiredI = slot.GetHash() & mask;

				const auto distDesired = (i - desiredI) & mask;
				const auto distHole = (i - holeI) & mask;

				if (distHole <= distDesired) // The element is closer to the hole than it is to its desired spot, we can move it back
				{
					auto& data = _slots[i].GetData();
					_slots[holeI].EmplaceData(_slots[i].GetHash(), std::move_if_noexcept(data.Key), std::move_if_noexcept(data.Value));

					_slots[i].ClearData();

					// The hole has moved!
					holeI = i;
				}

				i = (i + 1) & mask;
			}

			return true;
		}

	private:
		Memory::Allocator* _allocator;
		DeletionStrategy _deletionStrategy;
		mutable HashAlgorithm _hashAlgorithm;
		Span<Slot> _slots { };
		size_t _count { 0 };
		size_t _tombstoneCount { 0 };
		size_t _resizeThreshold { 0 };
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
