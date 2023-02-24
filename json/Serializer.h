#pragma once

#include <json/json.h>

#include <type_traits>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

namespace serializer {
namespace json {

struct Serializer : traits::SerializerTraits<false> {
private:
	Json::Value node;
public:
	Serializer(Json::Value const& _node = {}) : node(_node) {}

	Serializer operator[](std::string const& name) {
		return Serializer{node[name]};
	}

	Json::Value const& getNode() const {
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
		} else if constexpr (traits::is_map_w_key_v<std::string, value_type>) {
			for (auto& [k, v] : t) {
				Serializer val_ser;
				val_ser % v;
				node[k] = std::move(val_ser.getNode());
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
			node.append(serializer.getNode());
		}
	}
};

}
}


