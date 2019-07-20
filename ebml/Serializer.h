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
struct Serializer {
private:
	Buffer buffer;
	Serializer* parent {nullptr};
	std::size_t autoIdLen;
	bool finalized {false};
	Varint id;

	void write_raw(std::vector<std::byte> const& data) {
		copy(begin(data), end(data), back_inserter(buffer));
	}

public:
	Serializer(std::size_t _autoIdLen=4, Serializer* _parent=nullptr)
		: parent{_parent}
		, autoIdLen{_autoIdLen}
	{
		if (not parent) {
			// if this is the root element we need to write an ebml header
			auto headerSer = (*this)[0x1A45DFA3];
			headerSer[0x4286] % 1; // ebml version
			headerSer[0x42f7] % 1; // ebml reader version
			headerSer[0x42f2] % autoIdLen; // maximum id-length
			headerSer[0x42f3] % 8; // maximum size-length
			headerSer[0x4282] % std::string{"ebml-serializer"}; // maximum size-length
		}
	}

	Serializer(Varint const& _id, std::size_t _autoIdLen=4, Serializer* _parent=nullptr)
		: Serializer(_autoIdLen, _parent)
	{ id = _id; }

	~Serializer()
	{
		if (parent) {
			parent->write_raw(id);
			parent->write_raw(toVarint(buffer.size()));
			parent->write_raw(buffer);
		}
	}

	Serializer operator[](std::uint64_t id) {
		auto numBytes = detail::getSignificantBytes(id);
		Varint vint;
		do {
			--numBytes;
			vint.emplace_back(std::byte((id >> (8*numBytes)) & 0xff));
		} while (numBytes);
		return (*this)[vint];
	}

	Serializer operator[](std::string_view const& name) {
		return (*this)[genID<Hasher>(name, autoIdLen)];
	}

	Serializer operator[](Varint const& id) {
		return Serializer(id, autoIdLen, this);
	}

	auto getBuffer() const -> decltype(buffer) const& { return buffer; }

	template<typename T>
	void operator%(T t) {
		using value_type = std::remove_reference_t<std::remove_cv_t<T>>;
		if constexpr (std::is_same_v<value_type, std::string> or std::is_same_v<value_type, std::string_view>) {
			transform(begin(t), end(t), std::back_inserter(buffer), [](auto c) {return std::byte(c);});
		} else if constexpr (std::is_integral_v<value_type>) {
			auto numBytes = detail::getSignificantBytes(t);
			do {
				--numBytes;
				buffer.emplace_back(std::byte((t >> (8*numBytes)) & 0xff));
			} while (numBytes);
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
			Serializer({std::byte{0x81}}, autoIdLen, this) % *begin;
		}
	}
};
}

using Serializer = detail::Serializer<detail::Hash>;
}
}


