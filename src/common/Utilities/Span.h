#pragma once
#include <vector>
#include <array>
#include "Errors.h"
#include "TypeTraits.h"

/*********************************************************
* ---Span---
*
* Implements a lightweigh read-only view for continous 
* data, ie. from arrays, std::vector<> ...
*
* --FEATURES:
* Supports iteration, size-checks and data-access in STL-style
* Accepts any array-like data that can eigther be represented by
    eigther a value-pointer and a size or
    any valid pair of random-access iterators
*
* --IMPORTANT NOTES:
* On construction, a pointer to the data is stored without
* copying, as such the view will be in an invalid state
* if it outlives the data-source. (so don't use it as a
* longterm data storage unless you know what you're doing)
*
* --EXAMPLE USE-CASES:
*
* Replace usafe "T* array, size_t size" pairs in functions
* Replace unsafe usage of T* as return-value for arrays
* Replace "const Container<T>&" as input/output for functions
  (ie. allows to replace container type w/o changing signature)
*
* -- REFERENCE:
*
* http://www.boost.org/doc/libs/develop/libs/beast/doc/html/beast/ref/boost__beast__span.html
**********************************************************/

template<typename Type, size_t Size>
using StaticArray = const Type(&)[Size];

template<typename Type>
class span
{
public:
	using size_type = size_t;
	using iterator = const Type*;
    using const_iterator = const Type*;
	using value_type = const Type;

	constexpr span(void) noexcept :
		m_pArray(nullptr), m_size(0)
	{
	}

	constexpr span(std::nullptr_t) noexcept :
        span()
	{
	}

    constexpr span(span&& s) :
        span(s.m_pArray, s.m_size) 
    {
        s.m_pArray = nullptr;
        s.m_size = 0;
    }

	template<size_t size>
	constexpr span(StaticArray<Type, size> pArray) noexcept :
        span(pArray, size)
	{
	}

	constexpr span(const Type* pArray, size_t size) noexcept :
		m_pArray(pArray), m_size(size)
	{
	}

    span(const std::vector<Type>& vector) noexcept :
        span(vector.data(), vector.size())
	{
	}

	template<size_t size>
    constexpr span(const std::array<Type, size>& array) noexcept :
        span(array.data(), size)
	{
	}

	// TODO: potentially unsafe?
	constexpr span(const std::initializer_list<Type>& list) noexcept :
		m_pArray(list.begin()), m_size(list.size())
	{
	}

	constexpr span(const span& view) = default;

	template<typename OtherType>
	constexpr span(const span<OtherType>& other) noexcept :
		m_pArray(other.data()), m_size(other.size())
	{
	}

	bool operator==(const span& other) const
	{
		if(m_size != other.m_size)
			return false;
		else
		{
			for(const auto [element1, element2] : iterateMulti(*this, other))
			{
				if(element1 != element2)
					return false;
			}
		}

		return true;
	}

    value_type* data(void) const
	{
		return m_pArray;
	}

	constexpr size_type size(void) const
	{
		return m_size;
	}

	constexpr bool empty(void) const
	{
		return m_size == 0;
	}

    value_type& front(void) const
	{
		return m_pArray[0];
	}

    value_type& back(void) const
	{
		return m_pArray[size() - 1];
	}

    value_type* begin(void) const
	{
		return m_pArray;
	}

    value_type* end(void) const
	{
		return m_pArray + m_size;
	}

	auto rbegin(void) const
	{
		return std::reverse_iterator<Type*>(end());
	}

	auto rend(void) const
	{
		return std::reverse_iterator<Type*>(begin());
	}

    value_type& operator[](size_type size) const
	{
#ifndef NDEBUG
        ASSERT(size < m_size);
#endif

		return m_pArray[size];
	}

	value_type& at(size_type size) const
	{
        ASSERT(size < m_size);

		return m_pArray[size];
	}

	void clear(void)
	{
		m_pArray = nullptr;
		m_size = 0;
	}

    // shrinks the view to (array+1, size)
	void pop_front(void)
	{
		ASSERT(!empty());
				
		m_pArray++;
		m_size--;
	}

    // shrinks the view to (array, size-1)
	void pop_back(void)
	{
        ASSERT(!empty());

		m_size--;
	}

	explicit constexpr operator bool() const
	{
		return !empty();
	}

private:

	value_type* m_pArray;
	size_t m_size;
};

/***********************************
* Types
************************************/

template<typename Container>
using container_span = span<typename Container::value_type>;

/***********************************
* HelperFunctions
************************************/

template<typename Type>
auto make_span(const std::vector<Type>& vType)
{
	return span<Type>(vType);
}

template<typename Type, size_t Size>
constexpr auto make_span(const std::array<Type, Size>& vType)
{
	return span<Type>(vType);
}

template<typename Type, CheckIsNoArray<Type> = 0>
constexpr auto make_span(const Type& value)
{
	return span<Type>(&value, 1);
}

template<typename Type, CheckIsQualifiedArray<Type> = 0>
constexpr auto make_span(const Type& rawArray)
{
	constexpr auto extent = std::extent_v<Type>;

	return span<std::remove_all_extents_t<Type>>(rawArray, extent);
}

template<typename Type>
constexpr auto make_span(Type* pFirst, Type* pSecond)
{
	ASSERT(pSecond > pFirst);

	return span<Type>(pFirst, pSecond - pFirst);
}

template<typename Type>
constexpr auto make_span(Type* pArray, size_t size)
{
	return span<Type>(pArray, size);
}

template<typename Iterator>
constexpr auto make_span(Iterator begin, Iterator end)
{
    static_assert(std::is_same_v<std::iterator_traits<Iterator>::iterator_category, std::random_access_iterator_tag>, "Only accepts random-access iterators.");

	ASSERT(begin <= end);

	return span<typename Iterator::value_type>(&*begin, std::distance(begin, end));
}
