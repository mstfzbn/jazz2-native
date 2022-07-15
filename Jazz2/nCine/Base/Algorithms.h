#pragma once

#include "Iterator.h"

#include <algorithm>

namespace nCine
{
	// Traits
	template <class T>
	struct isIntegral
	{
		static constexpr bool value = false;
	};
	template <>
	struct isIntegral<bool>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<char>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<unsigned char>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<short int>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<unsigned short int>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<int>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<unsigned int>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<long>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<unsigned long>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<long long>
	{
		static constexpr bool value = true;
	};
	template <>
	struct isIntegral<unsigned long long>
	{
		static constexpr bool value = true;
	};

	/// Returns true if the range is sorted into ascending order
	template <class Iterator>
	inline bool isSorted(Iterator first, const Iterator last)
	{
		if (first == last)
			return true;

		Iterator next = first;
		while (++next != last) {
			if (*next < *first)
				return false;
			++first;
		}

		return true;
	}

	/// Returns true if the range is sorted, using a custom comparison
	template <class Iterator, class Compare>
	inline bool isSorted(Iterator first, const Iterator last, Compare comp)
	{
		if (first == last)
			return true;

		Iterator next = first;
		while (++next != last) {
			if (comp(*next, *first))
				return false;
			++first;
		}

		return true;
	}

	/// Returns an iterator to the first element in the range which does not follow an ascending order, or last if sorted
	template <class Iterator>
	inline const Iterator isSortedUntil(Iterator first, const Iterator last)
	{
		if (first == last)
			return first;

		Iterator next = first;
		while (++next != last) {
			if (*next < *first)
				return next;
			++first;
		}

		return last;
	}

	/// Returns an iterator to the first element in the range which does not follow the custom comparison, or last if sorted
	template <class Iterator, class Compare>
	inline const Iterator isSortedUntil(Iterator first, const Iterator last, Compare comp)
	{
		if (first == last)
			return first;

		Iterator next = first;
		while (++next != last) {
			if (comp(*next, *first))
				return next;
			++first;
		}

		return last;
	}

	/// Partition function for quicksort with iterators
	template <class Iterator, class Compare>
	inline Iterator partition(Iterator first, Iterator last, Compare comp)
	{
		Iterator pivot = last;

		while (first != last) {
			while (comp(*first, *pivot)) {
				++first;
				if (first == last)
					return first;
			}

			do {
				--last;
				if (first == last)
					return first;
			} while (!comp(*last, *pivot));

			std::swap(*first, *last);
			++first;
		}

		return first;
	}

	namespace {

		/// Quicksort implementation with random access iterators and custom compare function
		template <class Iterator, class Compare>
		inline void quicksort(Iterator first, Iterator last, RandomAccessIteratorTag, Compare comp)
		{
			int size = distance(first, last);
			if (size > 1) {
				Iterator p = prev(last);
				std::swap(*next(first, size / 2), *p);
				Iterator q = partition(first, p, comp);
				std::swap(*q, *p);
				quicksort(first, q, RandomAccessIteratorTag(), comp);
				quicksort(next(q), last, RandomAccessIteratorTag(), comp);
			}
		}

		/// Quicksort implementation with bidirectional iterators and custom compare function
		template <class Iterator, class Compare>
		inline void quicksort(Iterator first, Iterator last, BidirectionalIteratorTag, Compare comp)
		{
			if (first != last) {
				Iterator p = prev(last);
				swap(*first, *p);
				Iterator q = partition(first, p, comp);
				swap(*q, *p);
				quicksort(first, q, BidirectionalIteratorTag());
				quicksort(next(q), last, BidirectionalIteratorTag());
			}
		}

	}

	/// Quicksort implementation with iterators and custom compare function
	template <class Iterator, class Compare>
	inline void quicksort(Iterator first, Iterator last, Compare comp)
	{
		quicksort(first, last, IteratorTraits<Iterator>::IteratorCategory(), comp);
	}

	/// Quicksort implementation with iterators, ascending order
	template <class Iterator>
	inline void quicksort(Iterator first, Iterator last)
	{
		quicksort(first, last, IteratorTraits<Iterator>::IteratorCategory(), IsLess<typename IteratorTraits<Iterator>::ValueType>);
	}

	/// Quicksort implementation with iterators, descending order
	template <class Iterator>
	inline void quicksortDesc(Iterator first, Iterator last)
	{
		quicksort(first, last, IteratorTraits<Iterator>::IteratorCategory(), IsNotLess<typename IteratorTraits<Iterator>::ValueType>);
	}

	/// A container for functions to destruct objects and arrays of objects
	template <bool value>
	struct destructHelpers
	{
		template <class T>
		inline static void destructObject(T* ptr)
		{
			ptr->~T();
		}

		template <class T>
		inline static void destructArray(T* ptr, unsigned int numElements)
		{
			for (unsigned int i = 0; i < numElements; i++)
				ptr[numElements - i - 1].~T();
		}
	};

	/// Specialization for trivially destructible types
	template <class T, typename = void>
	struct isDestructible
	{
		static constexpr bool value = false;
	};
	template <class T>
	struct isDestructible<T, decltype(declVal<T&>().~T())>
	{
		static constexpr bool value = (true && !__is_union(T));
	};

	template <class T>
	struct hasTrivialDestructor
	{
		static constexpr bool value = __has_trivial_destructor(T);
	};

	template <class T>
	struct isTriviallyDestructible
	{
		static constexpr bool value = isDestructible<T>::value && hasTrivialDestructor<T>::value;
	};

	template <>
	struct destructHelpers<true>
	{
		template <class T>
		inline static void destructObject(T* ptr)
		{
		}

		template <class T>
		inline static void destructArray(T* ptr, unsigned int numElements)
		{
		}
	};

	template <class T>
	void destructObject(T* ptr)
	{
		destructHelpers<isTriviallyDestructible<T>::value>::destructObject(ptr);
	}

	template <class T>
	void destructArray(T* ptr, unsigned int numElements)
	{
		destructHelpers<isTriviallyDestructible<T>::value>::destructArray(ptr, numElements);
	}

	inline float lerp(float a, float b, float ratio)
	{
		return a + ratio * (b - a);
	}

	inline int lerp(int a, int b, float ratio)
	{
		return (int)round(a + ratio * (float)(b - a));
	}
}
