#pragma once

#include <type_traits>
#include <algorithm>
#include <string_view>

#include "serializer/Converter.h"
#include "serializer/traits.h"

#include "hasher.h"
#include "varint.h"

namespace serializer {
namespace ebml {

using Buffer = std::vector<std::byte>;

namespace detail {

template<typename Hasher>
struct Serializer: traits::SerializerTraits<false> {
private:
	Buffer buffer;
	Serializer* parent {nullptr};
	std::size_t autoIdLen;
	std::optional<Varint> id;

    template<typename T>
	void write_raw(T const& t) {
		std::copy(std::begin(t), std::end(t), std::back_inserter(buffer));
	}

public:

	Serializer(std::size_t _autoIdLen=4)
		: autoIdLen{_autoIdLen}
	{
        if (_autoIdLen > 8) {
            throw std::invalid_argument("ebml allows for ids to be of length 8 maximum!");
        }
        // if this is the root element we need to write an ebml header
        auto headerSer = (*this)[0x0A45DFA3];
        headerSer[0x0286] % 1; // ebml version
        headerSer[0x02f7] % 1; // ebml reader version
        headerSer[0x02f2] % autoIdLen; // maximum id-length
        headerSer[0x02f3] % 8; // maximum size-length
        headerSer[0x0282] % std::string("ebml-serializer"); // name
	}

	Serializer(std::size_t _autoIdLen, Serializer* _parent)
		: parent{_parent}
		, autoIdLen{_autoIdLen}
	{
		if (not parent) {
            throw std::invalid_argument("need a parent serializer");
		}
	}

	Serializer(Varint const& _id, std::size_t _autoIdLen, Serializer* _parent)
		: Serializer(_autoIdLen, _parent)
	{ id = _id; }

	~Serializer()
	{
		if (parent and id) {
			parent->write_raw(*id);
			parent->write_raw(VarLen{buffer.size()});
			parent->write_raw(buffer);
		}
	}

	Serializer operator[](Varint const& id) {
		return Serializer(id, autoIdLen, this);
	}

	Serializer operator[](std::uint64_t id) {
		return (*this)[Varint{id}];
	}

	Serializer operator[](std::string_view const& name) {
		return (*this)[genID<Hasher>(name, autoIdLen)];
	}

	auto getBuffer() const -> decltype(buffer) const& { return buffer; }

	template<typename T>
	void operator%(T&& t) {
        if (not id) {
            throw std::runtime_error("cannot serialize into an EBML node without an ID");
        }
        buffer.clear();
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
		if constexpr (std::is_same_v<value_type, std::string> or std::is_same_v<value_type, std::string_view>) {
			transform(begin(t), end(t), std::back_inserter(buffer), [](auto c) {return std::byte(c);});
		} else if constexpr (std::is_integral_v<value_type>) {
			auto numBytes = detail::getOctetLength(t);
			while (numBytes) {
				--numBytes;
				buffer.emplace_back(std::byte((t >> (8*numBytes)) & 0xff));
			};
		} else if constexpr (std::is_enum_v<value_type>) {
			(*this) % static_cast<std::underlying_type_t<value_type>>(t);
		} else if constexpr (traits::has_serialize_function_v<value_type, decltype(*this)>) {
			t.serialize(*this);
		} else {
			// last resort is using a converter
			Converter<value_type> converter;
			converter.serialize(*this, t);
		}
	}

	template<typename IterT>
	void serializeSequence(IterT begin, IterT end) {
		for (; begin != end; std::advance(begin, 1)) {
			Serializer(Varint{0x01}, autoIdLen, this) % *begin;
		}
	}
};
}

using Serializer = detail::Serializer<detail::Hash>;
}
}


