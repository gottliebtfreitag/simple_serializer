#pragma once

#include <yaml-cpp/yaml.h>

#include <type_traits>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

namespace serializer {
namespace yaml {

struct Serializer {
private:
	YAML::Node node;
public:
	Serializer(YAML::Node const& _node = YAML::Node{}) : node(_node) {}

	Serializer operator[](std::string const& name) {
		return Serializer{node[name]};
	}

	YAML::Node const& getNode() const {
		return node;
	}

	template<typename T>
	void operator%(T&& t) {
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

		if constexpr (traits::has_serialize_function_v<value_type, decltype(*this)>) {
			t.serialize(*this);
		} else if constexpr (std::is_same_v<value_type, std::string>) {
			node = t;
		} else if constexpr (std::is_arithmetic_v<value_type>) {
			node = t;
		} else if constexpr (std::is_enum_v<value_type>) {
			node = static_cast<std::underlying_type_t<value_type>>(t);
		} else if constexpr (traits::is_map_v<value_type>) {
			for (auto& elem : t) {
				Serializer left, right;
				left % elem.first;
				right% elem.second;
				node[left.getNode()] = right.getNode();
			}
		} else {
			// last resort is using a converter
			Converter<value_type> converter;
			converter.serialize(*this, t);
		}
	}

	template<typename IterT>
	void serializeSequence(IterT begin, IterT end) {
		for (; begin != end; std::advance(begin, 1)) {
			Serializer serializer;
			serializer % *begin;
			node.push_back(serializer.getNode());
		}
	}
};

}
}


