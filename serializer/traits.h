#pragma once

#include <type_traits>

namespace serializer
{
namespace traits
{

template <typename T>
struct is_map {
private:
	template <typename KT, typename MT>
	static std::true_type test(std::map<KT, MT>);

	template <typename...>
	static std::false_type test(...);
public:
	using type = decltype(test(std::declval<T>()));
	enum { value = type::value };
};
template<typename T>
inline constexpr bool is_map_v = is_map<T>::value;

template <typename T>
struct is_pair {
private:
	template <typename KT, typename MT>
	static std::true_type test(std::pair<KT, MT>);

	template <typename...>
	static std::false_type test(...);
public:
	using type = decltype(test(std::declval<T>()));
	enum { value = type::value };
};
template<typename T>
inline constexpr bool is_pair_v = is_pair<T>::value;


template <typename T, typename Arg1>
struct has_serialize_function {
private:
	template <typename U>
	static decltype(std::declval<U>().serialize(std::declval<std::add_lvalue_reference_t<Arg1>>()), void(), std::true_type()) test(int);

	template <typename>
	static std::false_type test(...);
public:
	using type = decltype(test<T>(int(0)));
	enum { value = type::value };
};


template <typename T, typename Arg>
inline constexpr bool has_serialize_function_v = has_serialize_function<T, Arg>::value;


}
}


