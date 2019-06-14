#pragma once

#include <type_traits>
#include <algorithm>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

#include "varint.h"

namespace serializer {
namespace ebml {

template<typename Hasher=detail::Hash>
struct Deserializer {
	using size_t = std::make_signed_t<std::size_t>;
private:

	std::byte const* buffer;
	size_t size;
	std::size_t autoIdLen{4};

	Deserializer(std::byte const* _buffer, size_t _size, std::size_t _autoIdLen)
		: buffer{_buffer}, size{_size}, autoIdLen{_autoIdLen}
	{}

	using ChildInfo = std::pair<Varint, Deserializer>;
	std::optional<std::vector<ChildInfo>> childElements;

	void populateChildren() {
		if (not childElements) {
			std::vector<ChildInfo> children;
			auto b = buffer;
			auto endB = buffer + size;
			while (b < endB) {
				auto childID = readVarint(b, endB-b); // this moves b
				auto contentLen = varintToNumber(readVarint(b, endB-b));
				children.emplace_back(std::move(childID), Deserializer(b, contentLen, autoIdLen));
				b += contentLen;
			}
			childElements = std::move(children);
		}
	}

public:
	Deserializer(std::byte const* _buffer, std::size_t _size)
		: buffer{_buffer}, size{static_cast<size_t>(_size)}
	{
		// read the header
		auto headerDeser = (*this)[0x1A45DFA3];
		if (headerDeser.size == -1) {
			throw std::runtime_error("cannot deserialize stream! there is no header information");
		}
		headerDeser[0x42f2] % autoIdLen; // maximum id-length
		std::string contentType;
		headerDeser[0x4282] % contentType;
		if (contentType != "ebml-serializer") {
			throw std::runtime_error("cannot deserialize stream! wrong document type");
		}
	}

	Deserializer operator[](std::uint64_t id) {
		auto numBytes = detail::getSignificantBytes(id);
		Varint vint;
		do {
			--numBytes;
			vint.emplace_back(std::byte((id >> (8*numBytes)) & 0xff));
		} while (numBytes);
		return (*this)[vint];
	}

	Deserializer operator[](std::string_view const& name) {
		return (*this)[genID<Hasher>(name, autoIdLen)];
	}

	Deserializer operator[](Varint const& id) {
		populateChildren();
		auto it = std::find_if(childElements->begin(), childElements->end(),
				[&](auto c)
				{ return c.first == id; }
		);

		if (it == childElements->end()) {
			return Deserializer(buffer, -1, autoIdLen);
		}
		Deserializer ret = it->second;
		childElements->erase(it);
		return ret;
 	}


	template<typename T>
	void operator%(T& t) {
		using value_type = std::remove_cv_t<T>;
		if (size < 0) {
			return;
		}
		if constexpr (std::is_same_v<value_type, std::string>) {
			t = value_type(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(size));
		} else if constexpr (std::is_integral_v<value_type>) {
			t = 0;
			for (auto i{0U}; i < size; ++i) {
				t = (t << 8) | static_cast<value_type>(buffer[i]);
			}
		} else if constexpr (std::is_enum_v<value_type>) {
			(*this) % static_cast<std::underlying_type_t<value_type>>(t);
		} else if constexpr (traits::has_serialize_function_v<value_type, decltype(*this)>) {
			t.serialize(*this);
		} else {
			// last resort is using a converter
			Converter<value_type> converter;
			converter.deserialize(*this, t);
		}
	}

	template<typename T, typename ElemCb, typename CountCB=int>
	void deserializeSequence(ElemCb&& cb, CountCB&& countCB=CountCB{}) {
		populateChildren();
		Varint targetId{std::byte{0x81}};
		if constexpr (not std::is_same_v<CountCB, int>) {
			auto count = std::count_if(begin(*childElements), end(*childElements), [&](auto c) {return c.first == targetId; });
			countCB(count);
		}
		while (true) {
			auto subSer = (*this)[targetId];
			if (subSer.size == -1) {
				break;
			}
			T t;
			subSer % t;
			cb(std::move(t));
		}
	}
};

}
}


