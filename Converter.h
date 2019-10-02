#pragma once

#include <array>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <map>
#include <set>
#include <iostream>

#include "traits.h"

namespace serializer {


template<typename T, typename Specialized=void>
class Converter;

namespace detail {

template<typename T>
struct SequenceContainerConverter {
	using value_type = T;
	template<typename Serializer>
	void serialize(Serializer& adapter, value_type const& x) {
		adapter.serializeSequence(std::begin(x), std::end(x));
	}
	template<typename Deserializer>
	void deserialize(Deserializer& adapter, value_type& x) {
		using inner_type = typename value_type::value_type;
		x.clear();
		adapter. template deserializeSequence<inner_type>([&x](inner_type v) { x.emplace_back(std::move(v)); });
	}
};

template<typename T>
struct ContainerConverter {
	using value_type = T;
	template<typename Serializer>
	void serialize(Serializer& adapter, value_type const& x) {
		adapter.serializeSequence(std::begin(x), std::end(x));
	}
	template<typename Deserializer>
	void deserialize(Deserializer& adapter, value_type& x) {
		using inner_type = typename value_type::value_type;
		x.clear();
		adapter. template deserializeSequence<inner_type>([&x](inner_type v) { x.emplace(std::move(v)); });
	}
};

}

template<typename T>
struct Converter<std::vector<T>>       : detail::SequenceContainerConverter<std::vector<T>> {};
template<typename T>
struct Converter<std::list<T>>         : detail::SequenceContainerConverter<std::list<T>> {};
template<typename T>
struct Converter<std::basic_string<T>> : detail::SequenceContainerConverter<std::list<T>> {};

template<typename T>
struct Converter<std::set<T>> : detail::ContainerConverter<std::set<T>> {};

template<typename Key, typename Value>
struct Converter<std::pair<Key, Value>> {
	using value_type = std::pair<Key, Value>;
	template<typename Serializer>
	void serialize(Serializer& adapter, value_type const& x) {
		adapter["first"]  % x.first;
		adapter["second"] % x.second;
	}
	template<typename Deserializer>
	void deserialize(Deserializer& adapter, value_type& x) {
		adapter["first"]  % x.first;
		adapter["second"] % x.second;
	}
};

template<typename T>
struct Converter<T, typename std::enable_if<std::is_enum<T>::value>::type> {
	using value_type = typename std::underlying_type<T>::type;
	template<typename Serializer>
	void serialize(Serializer& adapter, T const& x) {
		value_type value = Type(x);
		adapter % value;
	}
	template<typename Deserializer>
	void deserialize(Deserializer& adapter, T& x) {
		value_type value;
		adapter % value;
		x = T(value);
	}
};

template<typename T>
struct Converter<T, typename std::enable_if<traits::is_map_v<T>>::type> {
	template<typename Serializer>
	void serialize(Serializer& adapter, T const& x) {
		adapter.serializeSequence(begin(x), end(x));
	}
	template<typename Deserializer>
	void deserialize(Deserializer& adapter, T& x) {
		using value_type = std::pair<typename T::key_type, typename T::mapped_type>;
		x.clear();
		adapter. template deserializeSequence<value_type>([&x](value_type v) {
			x[std::move(v.first)] = std::move(v.second);
		});
	}
};

}
