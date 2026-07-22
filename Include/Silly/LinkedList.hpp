#pragma once
#include "Memory/Allocator.hpp"

namespace Silly
{
	template<typename T>
	class LinkedList
	{
		struct NodeBase
		{
			NodeBase* Next;
			NodeBase* Prev;
		};

		struct Node : NodeBase
		{
			T Value;
#
			template<typename... Args>
			explicit Node(Args&&... args) : Value(std::forward<Args>(args)...) {}
		};

		template<bool isConst> class Iterator
		{
			template<bool> friend class Iterator;
			friend class LinkedList;

			using NodeBaseType = std::conditional_t<isConst, const NodeBase, NodeBase>;
			using NodeType = std::conditional_t<isConst, const Node, Node>;

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::conditional_t<isConst, const T, T>;
			using pointer = value_type*;
			using reference = value_type&;

			explicit Iterator() : _node(nullptr) {}

			explicit Iterator(NodeBaseType* node) : _node(node) {}

			Iterator(const Iterator<false>& other)
				requires(isConst)
				: _node(other._node) {}

			Iterator& operator=(const Iterator<false>& other)
				requires(isConst)
			{
				_node = other._node;

				return *this;
			}

			// The "requires" clause on the converting constructors is evaluated too late and the compiler deletes the default copy constructor and operator
			Iterator(const Iterator& other) = default;
			Iterator& operator=(const Iterator& other) = default;

			reference operator*() const
			{
				return static_cast<NodeType*>(_node)->Value;
			}

			pointer operator->() const
			{
				return &static_cast<NodeType*>(_node)->Value;
			}

			Iterator& operator++()
			{
				_node = _node->Next;
				return *this;
			}

			Iterator operator++(int)
			{
				const auto tmp = *this;
				operator++();
				return tmp;
			}

			Iterator& operator--()
			{
				_node = _node->Prev;
				return *this;
			}

			Iterator operator--(int)
			{
				const auto tmp = *this;
				operator--();
				return tmp;
			}

			[[nodiscard]] friend bool operator==(const Iterator& a, const Iterator& b)
			{
				return a._node == b._node;
			}

			[[nodiscard]] friend bool operator!=(const Iterator& a, const Iterator& b)
			{
				return a._node != b._node;
			}

		private:
			NodeBaseType* _node;
		};

		static_assert(std::bidirectional_iterator<Iterator<true>>);
		static_assert(std::bidirectional_iterator<Iterator<false>>);

	public:
		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		static constexpr Memory::Allocator::AllocSpec AllocSpec { .Size = sizeof(Node), .Alignment = alignof(Node) };

		iterator begin()
		{
			return iterator { _sentinel.Next };
		}

		iterator end()
		{
			return iterator { &_sentinel };
		}

		const_iterator begin() const
		{
			return const_iterator { _sentinel.Next };
		}

		const_iterator end() const
		{
			return const_iterator { &_sentinel };
		}

		LinkedList(Memory::Allocator& allocator) : _allocator(&allocator), _sentinel(&_sentinel, &_sentinel), _length(0) {}

		~LinkedList()
		{
			Clear();
		}

		LinkedList(const LinkedList&) = delete;
		LinkedList& operator=(const LinkedList&) = delete;

		LinkedList(LinkedList&& other) noexcept : _allocator(other._allocator), _sentinel(other._sentinel), _length(other._length)
		{
			if (other._length == 0)
			{
				// If other does not contain any nodes, the sentinel should be pointing at itself in both directions
				VERIFY(_sentinel.Next == &other._sentinel);
				VERIFY(_sentinel.Prev == &other._sentinel);

				// Fix sentinel pointers
				_sentinel.Next = &_sentinel;
				_sentinel.Prev = &_sentinel;
			}
			else
			{
				// If other Ddoes contain nodes, the sentinel should not be pointing at itself
				VERIFY(_sentinel.Next != &other._sentinel);
				VERIFY(_sentinel.Prev != &other._sentinel);

				// The element immediately following the sentinel should however be pointing at it
				VERIFY(_sentinel.Next->Prev == &other._sentinel);
				VERIFY(_sentinel.Prev->Next == &other._sentinel);

				// Fix sentinel pointers
				_sentinel.Next->Prev = &_sentinel;
				_sentinel.Prev->Next = &_sentinel;

				// Reset other for safe destruction
				other._sentinel.Next = &other._sentinel;
				other._sentinel.Prev = &other._sentinel;
				other._length = 0;
			}
		}

		LinkedList& operator=(LinkedList&& other) noexcept
		{
			if (&other == this)
				return *this;

			Clear();

			_allocator = other._allocator;
			_sentinel = other._sentinel;
			_length = other._length;

			if (other._length == 0)
			{
				VERIFY(_sentinel.Next == &other._sentinel);
				VERIFY(_sentinel.Prev == &other._sentinel);

				// Fix sentinel pointers
				_sentinel.Next = &_sentinel;
				_sentinel.Prev = &_sentinel;
			}
			else
			{
				VERIFY(_sentinel.Next != &other._sentinel);
				VERIFY(_sentinel.Prev != &other._sentinel);

				// Fix sentinel pointers
				_sentinel.Next->Prev = &_sentinel;
				_sentinel.Prev->Next = &_sentinel;

				// Reset other for safe destruction
				other._sentinel.Next = &other._sentinel;
				other._sentinel.Prev = &other._sentinel;
				other._length = 0;
			}

			return *this;
		}

		[[nodiscard]] size_t GetLength() const
		{
			return _length;
		}

		void Clear()
		{
			auto node = _sentinel.Next;
			while (node != &_sentinel)
			{
				const auto next = node->Next;
				DestroyNode(static_cast<Node*>(node));
				node = next;
			}

			_sentinel.Next = &_sentinel;
			_sentinel.Prev = &_sentinel;
			_length = 0;
		}

		template<typename... Args>
		[[nodiscard]] Result<iterator, Error> Prepend(Args&&... args)
		{
			return InsertBefore(begin(), std::forward<Args>(args)...);
		}

		template<typename... Args>
		[[nodiscard]] Result<iterator, Error> Append(Args&&... args)
		{
			return InsertBefore(end(), std::forward<Args>(args)...);
		}

		template<typename... Args>
		[[nodiscard]] Result<iterator, Error> InsertBefore(iterator before, Args&&... args)
		{
			const auto node = TRY(CreateNode(std::forward<Args>(args)...));
			InsertBeforeImpl(node, before._node);
			return Ok(iterator(node));
		}

		template<typename... Args>
		[[nodiscard]] Result<iterator, Error> InsertAfter(iterator after, Args&&... args)
		{
			const auto node = TRY(CreateNode(std::forward<Args>(args)...));
			InsertAfterImpl(node, after._node);
			return Ok(iterator(node));
		}

		[[nodiscard]] Result<iterator, Error> Remove(iterator it)
		{
			if (it == end())
				return Err(Error("Attempted to remove end() iterator"));

			const auto node = static_cast<Node*>(it._node);
			const auto next = node->Next;

			RemoveImpl(node);
			DestroyNode(node);
			return Ok(iterator(next));
		}

	private:
		FORCE_INLINE void InsertBeforeImpl(Node* node, NodeBase* before)
		{
			node->Next = before;
			node->Prev = before->Prev;
			InsertFinishImpl(node);
		}

		FORCE_INLINE void InsertAfterImpl(Node* node, NodeBase* after)
		{
			node->Next = after->Next;
			node->Prev = after;
			InsertFinishImpl(node);
		}
		;
		FORCE_INLINE void InsertFinishImpl(Node* node)
		{
			node->Next->Prev = node;
			node->Prev->Next = node;
			_length++;
		}

		FORCE_INLINE void RemoveImpl(Node* node)
		{
			node->Next->Prev = node->Prev;
			node->Prev->Next = node->Next;
			_length--;
		}

		template<typename... Args>
		[[nodiscard]] FORCE_INLINE Result<Node*, Error> CreateNode(Args&&... args)
		{
			const auto node = TRY(_allocator->Allocate<Node>());
			std::construct_at(node, std::forward<Args>(args)...);
			return Ok(node);
		}

		FORCE_INLINE void DestroyNode(Node* node)
		{
			_allocator->Delete(node);
		}

		Memory::Allocator* _allocator;
		NodeBase _sentinel;
		size_t _length;
	};
}
