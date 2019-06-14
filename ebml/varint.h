#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

namespace serializer {
namespace ebml {

namespace detail {


template<typename T>
constexpr std::size_t getSignificantBytes(T val) {
	if constexpr (std::is_unsigned_v<T>) {
		std::size_t numBytes = 0;
		for (; numBytes < sizeof(val); ++numBytes) {
			if (val < (1ULL<<(8*numBytes))) {
				break;
			}
		}
		return numBytes;
	} else {
		if (val >= 0) {
			return getSignificantBytes(static_cast<std::make_unsigned_t<T>>(val));
		} else {
			return getSignificantBytes(static_cast<std::make_unsigned_t<T>>(-val * 2 - 1));
		}
	}
}

}

using Varint = std::vector<std::byte>;

Varint toVarint(std::uint64_t val, std::size_t minNumBytes=1);

Varint readVarint(std::byte const* &buf, std::size_t len);
std::uint64_t varintToNumber(Varint const& vint);

template<typename Hasher, typename T>
Varint genID(T const& t, int maxIdLen) {
	auto hash = Hasher{}(t);
	hash = hash & ((1ULL<<(maxIdLen*7))-1);
	return toVarint(hash, maxIdLen);
}

}
}
