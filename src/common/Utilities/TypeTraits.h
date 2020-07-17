#pragma once
#include <type_traits>

// EnableIf

template<bool Test, typename Value = int>
using EnableIf = typename std::enable_if<Test, Value>::type;

template<typename T, typename T2, bool Compare = true, typename Value = int>
using CheckIsSame = EnableIf<std::is_same_v<T, T2> == Compare, Value>;

template<typename T, typename T2, bool Compare = true, typename Value = int>
using CheckIsBase = EnableIf<std::is_base_of_v<T, T2> == Compare, Value>;

template<typename T, bool Compare = true, typename Value = int>
using CheckIsPolymorphic = EnableIf<std::is_polymorphic_v<T> == Compare, Value>;

template<typename T, bool IsQualified, typename Value = int>
using CheckIsArray = EnableIf<std::is_array_v<T> && ((std::extent_v<T> != 0) == IsQualified), Value>;

template<typename T, typename Value = int>
using CheckIsQualifiedArray = CheckIsArray<T, true, Value>;

template<typename T, typename Value = int>
using CheckIsUnqualifiedArray = CheckIsArray<T, false, Value>;

template<typename T, typename Value = int>
using CheckIsNoArray = EnableIf<!std::is_array_v<T>, Value>;

// IsEnumClass


namespace impl
{
	template<typename T>
	auto isEnumClass(int) -> decltype((void)+T{}, std::false_type{});

	template<typename T>
	auto isEnumClass(...) -> std::true_type;
}

template<typename T>
constexpr bool IsEnumClass = decltype(impl::isEnumClass<T>(0))::value && std::is_enum_v<T>;

// variadic templates

template<typename... Args>
constexpr size_t ArgCount = sizeof...(Args);
template<typename... Args>
constexpr bool HasArgs = ArgCount<Args...> != 0;