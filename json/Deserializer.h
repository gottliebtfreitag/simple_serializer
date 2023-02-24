#pragma once

#include <json/json.h>

#include <type_traits>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

namespace serializer {
namespace json {

struct Deserializer : traits::SerializerTraits<true> {
private:
	Json::Value node;
public:
	Deserializer(Json::Value const& _node) : node(_node) {}

	Deserializer operator[](std::string const& name) {
		return node[name];
	}

	Json::Value const& getNode() const {
		return node;
	}

	template<typename T>
	void operator%(T& t) {
		if constexpr (traits::has_serialize_function_v<T, decltype(*this)>) {
			t.serialize(*this);
		} else if constexpr (std::is_same_v<T, std::string>) {
			t = node.asString();
		} else if constexpr (std::is_same_v<T, bool>) {
			t = node.asBool();
		} else if constexpr (std::is_integral_v<T>) {
			if constexpr (std::is_unsigned_v<T>) {
				t = node.asLargestUInt();
			} else {
				t = node.asLargestInt();
			}
		} else if constexpr (std::is_floating_point_v<T>) {
			t = node.asDouble();
		} else if constexpr (std::is_enum_v<T>) {
			std::underlying_type_t<T> ut{};
			(*this) % ut;
			t = static_cast<T>(ut);
		} else if constexpr (traits::is_map_w_key_v<std::string, T>) {
			t.clear();
			for (auto it = node.begin(); it != node.end(); ++it) {
				Deserializer val_deser{*it};
				typename T::mapped_type mt;
				val_deser % mt;
				t.emplace(it.name(), std::move(mt));
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


