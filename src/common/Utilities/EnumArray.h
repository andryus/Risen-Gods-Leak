#pragma once
#include <array>
#include "IterateHelper.h"

// Implements a static array using an enum class as Key
// enum class must have SIZE defined as the last element
// all of its values must be sequencial

template<typename Type, typename Enum>
class EnumArray
{
public:

    using Array = std::array<Type, (size_t)Enum::SIZE>;

    using value_type = Type;
    using iterator = typename Array::iterator;
    using const_iterator = typename Array::const_iterator;

    constexpr EnumArray(void) :
        m_array()
    {
    }

    constexpr EnumArray(std::initializer_list<Type> list)
    {
        /*for (auto[element, target] : iteratePair(list, m_array))
        {
            target = std::move(element);
        }*/

        for (uint8 i = 0; i < m_array.size(); i++)
        {
            m_array[i] = *(list.begin() + i);
        }
    }

    constexpr Type& operator[](Enum key)
    {
        return m_array[GetKey(key)];
    }

    constexpr const Type& operator[](Enum key) const
    {
        return m_array[GetKey(key)];
    }

    constexpr Type& at(Enum key)
    {
        return m_array.at(GetKey(key));
    }

    constexpr const Type& at(Enum key) const
    {
        return m_array.at(GetKey(key));
    }

    constexpr auto begin(void)
    {
        return m_array.begin();
    }

    constexpr auto end(void)
    {
        return m_array.end();
    }

    constexpr auto begin(void) const
    {
        return m_array.begin();
    }

    constexpr auto end(void) const
    {
        return m_array.end();
    }

    static constexpr size_t Size(void)
    {
        return (size_t)Enum::SIZE;
    }

    constexpr size_t size(void) const
    {
        return Size();
    }

    void Fill(const Type& value)
    {
        for (auto& element : *this)
        {
            element = value;
        }
    }

    constexpr auto front() const
    {
        return m_array.front();
    }

    constexpr auto back() const
    {
        return m_array.back();
    }

private:

    constexpr static size_t GetKey(Enum key)
    {
        return (size_t)key;
    }

    Array m_array;
};