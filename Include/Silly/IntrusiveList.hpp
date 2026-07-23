#pragma once
#include "Macros.hpp"

namespace Silly
{
	template<typename T> class IntrusiveList;

	template<typename T>
	struct IntrusiveNode
	{
		friend class IntrusiveList<T>;

		explicit IntrusiveNode() = default;

		FORCE_INLINE ~IntrusiveNode()
		{
#if!defined(NDEBUG)
			VERIFY(!_list);
#endif

			VERIFY(!_next && !_prev);
		}

		IntrusiveNode(const IntrusiveNode&)
		{
			// When a node is copy-constructed the new copy does not become part of the list if the original node was in one
		}

		IntrusiveNode& operator=(const IntrusiveNode&) // NOLINT(*-unhandled-self-assignment)
		{
			// When a node is copy-assigned from or to it remains in whatever list it was in, or remains listless if it was before
			return *this;
		}

		// Same ideas as for copying apply, we can move to and from a node that's in the list
		// Doesn't change the in-/out-of-list state of either the moved to or the moved from node
		IntrusiveNode(IntrusiveNode&&) noexcept {}

		IntrusiveNode& operator=(IntrusiveNode&&) noexcept
		{
			return *this;
		}

		[[nodiscard]] FORCE_INLINE T* GetNext()
		{
			if (_next)
				return Some(_next);

			return None;
		}

		[[nodiscard]] FORCE_INLINE T* GetPrev()
		{
			if (_prev)
				return Some(_prev);

			return None;
		}

		[[nodiscard]] FORCE_INLINE const T* GetNext() const
		{
			return _next;
		}

		[[nodiscard]] FORCE_INLINE const T* GetPrev() const
		{
			return _prev;
		}

	private:
		T* _next { nullptr };
		T* _prev { nullptr };

#if !defined(NDEBUG)
		void* _list { nullptr };
#endif
	};

	template<typename T>
	class IntrusiveList final
	{
		// Assert adherance to CRTP (struct T : IntrusiveNode<T>)
		static_assert(std::is_base_of_v<IntrusiveNode<T>, T>);

		template<bool isConst> class Iterator
		{
			template<bool> friend class Iterator;
			friend class IntrusiveList;

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::conditional_t<isConst, const T, T>;
			using pointer = value_type*;
			using reference = value_type&;

			explicit Iterator() : _node(nullptr), _tail(nullptr) {}
			explicit Iterator(T* node, T* tail) : _node(node), _tail(tail) {}

			Iterator(const Iterator<false>& other)
				requires(isConst)
				: _node(other._node), _tail(other._tail) {}

			Iterator& operator=(const Iterator<false>& other)
				requires(isConst)
			{
				_node = other._node;
				_tail = other._tail;

				return *this;
			}

			// The "requires" clause on the converting constructors is evaluated too late and the compiler deletes the default copy constructor and operator
			Iterator(const Iterator& other) = default;
			Iterator& operator=(const Iterator& other) = default;

			reference operator*() const
			{
				VERIFY(_node);
				return *_node;
			}

			pointer operator->() const
			{
				VERIFY(_node);
				return _node;
			}

			Iterator& operator++()
			{
				_node = _node->_next;
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
				_node = _node ? _node->_prev : _tail;
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
			T* _node;
			T* _tail;
		};

		static_assert(std::bidirectional_iterator<Iterator<true>>);
		static_assert(std::bidirectional_iterator<Iterator<false>>);

	public:
		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		iterator begin()
		{
			return iterator { _head, _tail };
		}

		iterator end()
		{
			return iterator { nullptr, _tail };
		}

		const_iterator begin() const
		{
			return const_iterator { _head, _tail };
		}

		const_iterator end() const
		{
			return const_iterator { nullptr, _tail };
		}

		IntrusiveList() = default;
		IntrusiveList(const IntrusiveList&) = delete;
		IntrusiveList& operator=(const IntrusiveList&) = delete;

		IntrusiveList(IntrusiveList&& other) noexcept
			: _head(std::exchange(other._head, nullptr)),
			  _tail(std::exchange(other._tail, nullptr)),
			  _length(std::exchange(other._length, 0))
		{
#if !defined(NDEBUG)
			for (auto node = _head; node; node = node->_next)
				node->_list = this;
#endif
		}

		[[nodiscard]] FORCE_INLINE T* GetHead()
		{
			return _head;
		}

		[[nodiscard]] FORCE_INLINE T* GetTail()
		{
			return _tail;
		}

		[[nodiscard]] FORCE_INLINE const T* GetHead() const
		{
			return _head;
		}

		[[nodiscard]] FORCE_INLINE const T* GetTail() const
		{
			return _tail;
		}

		IntrusiveList& operator=(IntrusiveList&& other) noexcept
		{
			if (&other == this)
				return *this;

			Clear();
			_head = std::exchange(other._head, nullptr);
			_tail = std::exchange(other._tail, nullptr);
			_length = std::exchange(other._length, 0);

#if !defined(NDEBUG)
			for (auto node = _head; node; node = node->_next)
				node->_list = this;
#endif

			return *this;
		}

		FORCE_INLINE ~IntrusiveList()
		{
			Clear();
		}

		[[nodiscard]] FORCE_INLINE size_t GetLength() const
		{
			return _length;
		}

		[[nodiscard]] FORCE_INLINE bool IsEmpty() const
		{
			return _length == 0;
		}

		void Clear()
		{
			for (auto node = _head; node;)
			{
				const auto next = node->_next;
#if !defined(NDEBUG)
				node->_list = nullptr;
#endif
				node->_next = nullptr;
				node->_prev = nullptr;
				node = next;
			}

			_head = nullptr;
			_tail = nullptr;
			_length = 0;
		}

		FORCE_INLINE void Prepend(T* node)
		{
			VerifyFreeNode(node);
			node->_next = _head;
			InsertFinishImpl(node);
		}

		FORCE_INLINE void Append(T* node)
		{
			VerifyFreeNode(node);
			node->_prev = _tail;
			InsertFinishImpl(node);
		}

		FORCE_INLINE void InsertBefore(T* before, T* node)
		{
			VerifyFreeNode(node);
			VerifyMemberNode(before);
			InsertBeforeImpl(before, node);
		}

		FORCE_INLINE void InsertAfter(T* after, T* node)
		{
			VerifyFreeNode(node);
			VerifyMemberNode(after);
			InsertAfterImpl(after, node);
		}

		FORCE_INLINE void Remove(T* node)
		{
			VerifyMemberNode(node);
			RemoveImpl(node);
		}

		FORCE_INLINE void VerifyMemberNode(T* node)
		{
			VERIFY(node);
#if !defined(NDEBUG)
			VERIFY(node->_list == this);
#endif
			VERIFY(node->_next || node->_prev || _length == 1);
		}

		static FORCE_INLINE void VerifyFreeNode(T* node)
		{
			VERIFY(node);
#if !defined(NDEBUG)
			VERIFY(!node->_list);
#endif
			VERIFY(!node->_next && !node->_prev);
		}

	private:
		FORCE_INLINE void InsertBeforeImpl(T* before, T* node)
		{
			node->_next = before;
			node->_prev = before->_prev;
			InsertFinishImpl(node);
		}

		FORCE_INLINE void InsertAfterImpl(T* after, T* node)
		{
			node->_next = after->_next;
			node->_prev = after;
			InsertFinishImpl(node);
		}

		FORCE_INLINE void InsertFinishImpl(T* node)
		{
			if (node->_prev)
			{
				node->_prev->_next = node;
			}
			else
			{
				VERIFY(_head == node->_next);
				_head = node;
			}

			if (node->_next)
			{
				node->_next->_prev = node;
			}
			else
			{
				VERIFY(_tail == node->_prev);
				_tail = node;
			}

#if !defined(NDEBUG)
			node->_list = this;
#endif

			_length++;
		}

		FORCE_INLINE void RemoveImpl(T* node)
		{
			if (node->_next)
			{
				node->_next->_prev = node->_prev;
			}
			else
			{
				VERIFY(_tail == node);
				_tail = node->_prev;
			}

			if (node->_prev)
			{
				node->_prev->_next = node->_next;
			}
			else
			{
				VERIFY(_head == node);
				_head = node->_next;
			}

#if !defined(NDEBUG)
			node->_list = nullptr;
#endif
			node->_next = nullptr;
			node->_prev = nullptr;

			_length--;
		}

		T* _head = nullptr;
		T* _tail = nullptr;
		size_t _length { 0 };
	};
}

#if SILLY_GLOBAL
using namespace Silly;
#endif
