#pragma once
#include <utility>
#include <tuple>
#include "Errors.h"

// defines multiple helper classes for iteration
// allows wider usage of range-based for loops ie. iterateRange(5, 15)


/***************************
* Index
***************************/

template<typename Container>
struct IndexIteratorHelper
{
    using IteratorType = typename Container::iterator;
    using ValueType = typename Container::value_type;

    struct Iterator
    {
        Iterator(IteratorType begin) :
            Iterator(begin, begin)
        {
        }

        Iterator(IteratorType begin, IteratorType self) :
            m_begin(begin), m_self(self)
        {
        }

        const Iterator& operator++(void)
        {
            m_self++;
            return *this;
        }

        std::pair<ValueType&, size_t> operator*(void) const
        {
            // TODO: for InputInterator, store & increment value
            return { *m_self, std::distance(m_begin, m_self) };
        }

        bool operator!=(const Iterator& iterator) const
        {
            ASSERT(m_begin == iterator.m_begin);

            return m_self != iterator.m_self;
        }

    private:

        IteratorType m_begin, m_self;
    };

    IndexIteratorHelper(Container& container) :
        m_container(container)
    {
    }

    auto begin() const
    {
        return Iterator(m_container.begin());
    }

    auto end() const
    {
        return Iterator(m_container.begin(), m_container.end());
    }

    auto Size(void) const
    {
        return m_container.Size();
    }

private:

    Container & m_container;
};

template<typename Container>
auto iterateIndex(Container& container)
{
    return IndexIteratorHelper<Container>(container);
}

/***************************
* Multi
***************************/

template<typename... Container>
struct MultiIteratorHelper
{
    template<typename... Iterators>
    struct Iterator
    {
        constexpr Iterator(Iterators&&... itrs) :
            m_itrs(std::make_tuple(itrs...))
        {
        }

        constexpr const Iterator& operator++(void)
        {
            Increment(std::index_sequence_for<Container...>());

            return *this;
        }

        constexpr auto operator*(void) const
        {
            return Deref(std::index_sequence_for<Container...>());
        }

        constexpr bool operator!=(const Iterator& iterator) const
        {
            return m_itrs != iterator.m_itrs;
        }

    private:

        template<size_t... Indices>
        constexpr void Increment(std::index_sequence<Indices...>)
        {
            (++Get<Indices>() , ...);
        }

        template<size_t... Indices>
        constexpr auto Deref(std::index_sequence<Indices...>) const
        {
            return std::tie(*Get<Indices>()...);
        }

        template<uint32 Index>
        constexpr decltype(auto) Get()
        {
            return std::get<Index>(m_itrs);
        }
        template<uint32 Index>
        constexpr decltype(auto) Get() const
        {
            return std::get<Index>(m_itrs);
        }

        std::tuple<Iterators...> m_itrs;
    };

    constexpr MultiIteratorHelper(Container&... container) :
        m_container(std::tie(container...))
    {
        AssertEqualSize();
    }

    constexpr auto begin()
    {
        return std::apply(Begin, m_container);
    }

    constexpr auto end()
    {
        return std::apply(End, m_container);
    }

    constexpr auto begin() const
    {
        return std::apply(Begin, m_container);
    }

    constexpr auto end() const
    {
        return std::apply(End, m_container);
    }

    constexpr auto size(void) const
    {
        AssertEqualSize();

        return std::size(std::get<0>(m_container));
    }

private:

    bool EqualsSize(const Container&... container) const
    {
        const auto size = std::size(std::get<0>(m_container));

        return ((std::size(container) == size) && ...);
    }

    template<typename... Iterators>
    static auto createIterator(Iterators&&... itr)
    {
        return Iterator<Iterators...>(std::move(itr)...);
    }

    static auto Begin(Container&... container)
    {
        return createIterator(std::begin(container)...);
    }

    static auto End(Container&... container)
    {
        return createIterator(std::end(container)...);
    }

    void AssertEqualSize() const
    {
        const bool equals = std::apply([this](const Container&... container){ return EqualsSize(container...); }, m_container);

        ASSERT(equals);
    }

    std::tuple<Container&...> m_container;
};

template<typename ... Container>
constexpr auto iterateMulti(Container&... container)
{
    return MultiIteratorHelper<Container...>(container...);
}

/***************************
* Count
***************************/

template<typename Type>
struct RangeIterateHelper
{
    struct Iterator
    {
        constexpr Iterator(Type value) :
            m_value(value)
        {
        }

        constexpr bool operator!=(const Iterator& iterator)
        {
            return m_value != iterator.m_value;
        }

        constexpr const Iterator& operator++(void)
        {
            m_value++;
            return *this;
        }

        constexpr decltype(auto) operator*(void) const
        {
            if constexpr(std::is_integral_v<Type>)
                return m_value;
            else
                return *m_value;
        }

    private:

        Type m_value;
    };

    constexpr RangeIterateHelper(Type first, Type last) :
        m_first(first), m_last(last)
    {
    }

    constexpr RangeIterateHelper(const RangeIterateHelper& range) = default;

    constexpr auto begin(void) const
    {
        return Iterator(m_first);
    }

    constexpr auto end(void) const
    {
        return Iterator(m_last);
    }

    constexpr auto Size(void) const
    {
        return m_last - m_first;
    }

    constexpr bool Shrink(void)
    {
        const auto isValid = m_first != m_last;

        m_last--;

        return isValid;
    }

private:

    Type m_first, m_last;
};

template<typename Type>
constexpr auto iterateRange(Type min, Type max)
{
    return RangeIterateHelper<Type>(min, max);
}

template<typename Type>
constexpr auto iterateTo(Type max)
{
    static_assert(std::is_integral_v<Type>, "Type must be an integral.");

    return iterateRange(Type(), max);
}

template<typename Container>
auto iterateContainerTo(const Container& vContainer, typename Container::size_type max)
{
    return RangeIterateHelper<typename Container::iterator>(vContainer.begin(), vContainer.begin() + max);
}

template<typename Container>
auto iterateContainerRange(const Container& vContainer, typename Container::size_type min, typename Container::size_type max)
{
    return iterateRange(vContainer.begin() + min, vContainer.begin() + max);
}

/***************************
* Enum
***************************/

template<typename Enum>
class EnumIteratorHelper
{
public:
    using Type = std::underlying_type_t<Enum>;

    class Iterator
    {
    public:

        constexpr Iterator(Enum value) :
            m_value(value)
        {
        }

        constexpr bool operator !=(const Iterator& other) const
        {
            return m_value != other.m_value;
        }

        constexpr const Iterator& operator++(void)
        {

            m_value = (Enum)(((Type)m_value) + 1);

            return *this;
        }

        constexpr Enum operator*(void) const
        {
            return m_value;
        }

    private:

        Enum m_value;
    };

    constexpr auto begin(void) const
    {
        return Iterator((Enum)0);
    }

    constexpr auto end(void) const
    {
        return Iterator(Enum::SIZE);
    }

    static constexpr auto Size(void)
    {
        return (Type)Enum::SIZE;
    }
};

template<typename Enum>
constexpr EnumIteratorHelper<Enum> iterateEnum(void)
{
    return EnumIteratorHelper<Enum>();
}

/***************************
* Pointer
***************************/

template<typename Type>
class PointerIteratorHelper
{
public:

    class Iterator
    {
    public:

        Iterator(Type* pValue) :
            m_pValue(pValue)
        {
        }

        bool operator !=(const Iterator& other) const
        {
            return m_pValue != other.m_pValue;
        }

        const Iterator& operator++(void)
        {
            m_pValue++;

            return *this;
        }

        Type* operator*(void) const
        {
            return m_pValue;
        }

    private:

        Type * m_pValue;
    };

    PointerIteratorHelper(Type* pData, size_t numElements) :
        m_pBegin(pData), m_pEnd(m_pBegin + numElements)
    {
    }

    auto begin(void) const
    {
        return Iterator(m_pBegin);
    }

    auto end(void) const
    {
        return Iterator(m_pEnd);
    }

    auto Size(void) const
    {
        return m_pEnd - m_pBegin;
    }

private:

    Type * m_pBegin, *m_pEnd;
};

template<typename Type>
PointerIteratorHelper<Type> iteratePointer(Type* pData, size_t numElements)
{
    return PointerIteratorHelper<Type>(pData, numElements);
}

/***************************
* Pointer
***************************/

class DataPointerIterateHelper
{
public:

    class Iterator
    {
    public:

        Iterator(char* pValue, size_t size) :
            m_pValue(pValue), m_size(size)
        {
        }

        bool operator !=(const Iterator& other) const
        {
            ASSERT(m_size == other.m_size);

            return m_pValue != other.m_pValue;
        }

        const Iterator& operator++(void)
        {
            m_pValue += m_size;

            return *this;
        }

        char* operator*(void) const
        {
            return m_pValue;
        }

    private:

        char* m_pValue;
        size_t m_size;
    };

    DataPointerIterateHelper(char* pData, size_t numElements, size_t elementSize) :
        m_pBegin(pData), m_pEnd(m_pBegin + numElements * elementSize), m_elementSize(elementSize)
    {
    }

    auto begin(void) const
    {
        return Iterator(m_pBegin, m_elementSize);
    }

    auto end(void) const
    {
        return Iterator(m_pEnd, m_elementSize);
    }

    auto Size(void) const
    {
        return m_pEnd - m_pBegin;
    }

private:

    char* m_pBegin, *m_pEnd;
    size_t m_elementSize;
};

inline DataPointerIterateHelper iterateDataPointer(char* pData, size_t numElements, size_t elementSize)
{
    return DataPointerIterateHelper(pData, numElements, elementSize);
}

/*************************************
* Reverse
**************************************/

template<typename T>
struct reversion_wrapper
{
    T& iterable;
};

template<typename T>
auto begin(reversion_wrapper<T> w)
{
    return std::rbegin(w.iterable);
}

template<typename T>
auto end(reversion_wrapper<T> w)
{
    return std::rend(w.iterable);
}

template<typename T>
reversion_wrapper<T> reverse(T&& iterable)
{
    return { iterable };
}
