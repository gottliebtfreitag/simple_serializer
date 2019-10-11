#pragma once

#include <yaml-cpp/yaml.h>

#include <type_traits>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

namespace serializer {
namespace yaml {

struct Deserializer {
private:
	YAML::Node node;
public:
	Deserializer(YAML::Node const& _node) : node(_node) {}

	Deserializer operator[](std::string const& name) {
		return node[name];
	}

	YAML::Node const& getNode() const {
		return node;
	}

	template<typename T>
	void operator%(T& t) {
		if constexpr (traits::has_serialize_function_v<T, decltype(*this)>) {
			t.serialize(*this);
		} else if constexpr (std::is_same_v<T, std::string>) {
			t = node.as<std::string>();
		} else if constexpr (std::is_arithmetic_v<T>) {
			t = node.as<T>();
		} else if constexpr (std::is_enum_v<T>) {
			t = static_cast<T>(node.as<std::underlying_type_t<T>>());
		} else if constexpr (traits::is_map_v<T>) {
			t.clear();
			for (auto const& v : node) {
				Deserializer left{v.first}, right{v.second};
				typename T::key_type kt;
				typename T::mapped_type mt;
				left % kt;
				right % mt;
				t.emplace(std::move(kt), std::move(mt));
			}
		} else {
			// last resort is using a converter
			Converter<T> converter;
			converter.deserialize(*this, t);
		}
	}

	template<typename T, typename ElemCb, typename CountCB=int>
	void deserializeSequence(ElemCb&& cb, CountCB&& countCB=CountCB{}) {
		static_assert(std::is_default_constructible_v<T>);
		if constexpr (std::is_invocable<CountCB, T>::value) {
			countCB(node.size());
		}
		for (auto c : node) {
			std::remove_cv_t<T> t;
			Deserializer deser{c};
			deser % t;
			cb(std::move(t));
		}
	}
};

}
}


