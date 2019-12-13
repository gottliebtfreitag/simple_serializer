#pragma once

#include <type_traits>

namespace serializer
{
namespace traits
{

template <typename T>
struct is_map : std::false_type {};
template <typename... Ts>
struct is_map<std::map<Ts...>> : std::true_type {};
template <typename... Ts>
struct is_map<std::unordered_map<Ts...>> : std::true_type {};
template<typename... Ts>
inline constexpr bool is_map_v = is_map<Ts...>::value;

template <typename T>
struct is_pair : std::false_type {};
template <typename... Ts>
struct is_pair<std::pair<Ts...>> : std::true_type {};
template<typename... Ts>
inline constexpr bool is_pair_v = is_pair<Ts...>::value;

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


