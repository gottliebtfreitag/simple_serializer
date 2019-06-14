#include "varint.h"
#include <stdexcept>

namespace serializer {
namespace ebml {

Varint toVarint(std::uint64_t val, std::size_t minNumBytes)
{
	Varint ret;
	minNumBytes = std::max(minNumBytes, detail::getSignificantBytes(val));
	std::byte msb  = static_cast<std::byte>(val >> ((minNumBytes-1)*8) & 0xff);
	std::byte head = static_cast<std::byte>(0x80 >> (minNumBytes-1));
	if (msb >= head) {
		ret.emplace_back(head>>1);
		ret.emplace_back(msb);
	} else {
		ret.emplace_back(head|msb);
	}
	for (int i = minNumBytes-2; i >= 0; --i) {
		ret.emplace_back(static_cast<std::byte>((val >> (i*8)) & 0xff));
	}
	return ret;
}

Varint readVarint(std::byte const* &buf, std::size_t len)
{
	if (len == 0) {
		return {};
	}
	std::byte head = *buf;
	auto addBytesToRead = sizeof(head) * 8;
	while (addBytesToRead--) {
		if (head >> (8-addBytesToRead) == std::byte(0x00)) {
			break;
		}
	}
	if (len < 1 + addBytesToRead) {
		return {};
	}
	auto beginOfVint = buf;
	buf += addBytesToRead+1;
	return Varint(beginOfVint, buf);
}

std::uint64_t varintToNumber(Varint const& vint) {
	if (vint.empty()) {
		return 0;
	}

	std::byte head = vint[0];
	auto addBytesToRead = sizeof(head) * 8;
	while (addBytesToRead--) {
		if (head >> (8-addBytesToRead) == std::byte(0x00)) {
			break;
		}
	}
	if (vint.size() != 1 + addBytesToRead) {
		throw std::runtime_error("invalid varint");
	}
	std::uint64_t ret{(static_cast<std::uint64_t>(head)&((1<<(7-addBytesToRead))-1))<<(addBytesToRead*8)};
	for (auto i{1U}; i < addBytesToRead; ++i) {
		ret = (ret << 8) | static_cast<std::uint64_t>(vint[i]);
	}
	return ret;
}

}
}
