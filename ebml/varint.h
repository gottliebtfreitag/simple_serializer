#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace serializer {
namespace ebml {
    
namespace detail 
{

template<typename T>
constexpr static std::size_t getOctetLength(T val) {
    if constexpr (std::is_unsigned_v<T>) {
        int octets{0};
        while (val) {
            ++octets;
            val >>= 8;
        }
        return octets;
    } else {
        if (val == 0) {
            return 0;
        }
        if (val > 0) {
            // the MSBit must not be set for positive numbers
            T ref = 0x7f;
            for (int i{1}; i < 9; ++i) {
                if (val <= ref) {
                    return i;
                }
                ref = (ref << 8) + 0xff;
            }
            return 8;
        } else {
            return getOctetLength(static_cast<std::make_unsigned_t<T>>((~val) + 1));
        }
    }
}

}

struct Varint {
private:
    std::array<std::byte, 8> buffer {std::byte{0x00}};
    std::size_t len {0};
    std::uint64_t val{0};

    static constexpr std::size_t calcOctetLen(std::uint64_t value) {
        std::size_t numBytes = 1;
        for (; numBytes < sizeof(value); ++numBytes) {
            if (0 == (value >> (7*numBytes))) {
                break;
            }
        }
        return numBytes;
    }

public:
    constexpr Varint(std::uint64_t value) noexcept
        : len{calcOctetLen(value)}
        , val{value}
    {
        for (std::size_t i{0}; i < len; ++i) {
            buffer[len-i-1] = std::byte{static_cast<unsigned char>((val >> (i*8)) & 0xff)};
        }
        std::byte head = static_cast<std::byte>(0x80 >> (len-1));
        buffer[0] |= head;
    }

    constexpr Varint(std::byte const* buf, std::size_t buf_len) {
        if (buf_len == 0) {
            return;
        }
        std::byte head = *buf;
        if (head == std::byte{0x00}) { // this would be an invalid varint
            throw std::domain_error("invalid varint header");
        }
        len = 1;
        while (true) {
            if (head >> (8-len) == std::byte(0x01)) {
                break;
            }
            ++len;
        }
        if (buf_len < len) {
            throw std::length_error("not enough bytes to unpack varint");
        }
        std::copy(buf, buf+len, buffer.begin());
        // decode the varint
        val = std::to_integer<decltype(val)>(buffer[0]) & ((1 << (8-len))-1);
        for (std::size_t i{1}; i < len; ++i) {
            val = (val << 8) + std::to_integer<decltype(val)>(buf[i]);
        }
    }

    constexpr Varint(Varint const&) noexcept = default;
    constexpr Varint& operator=(Varint const&) noexcept = default;

    constexpr operator std::uint64_t() const noexcept {
        return val;
    }

    constexpr auto value() const noexcept {
        return val;
    }

    constexpr auto data() const noexcept {
        return buffer.data();
    }
    constexpr auto data() noexcept {
        return buffer.data();
    }
    
    constexpr auto size() const noexcept {
        return len;
    }

    constexpr auto begin() noexcept {
        return buffer.begin();
    }
    constexpr auto begin() const noexcept {
        return buffer.begin();
    }
    constexpr auto end() noexcept {
        return buffer.begin() + size();
    }
    constexpr auto end() const noexcept {
        return buffer.begin() + size();
    }
};

template<typename Hasher, typename T>
constexpr Varint genID(T const& t, int maxIdLen) {
	auto hash = Hasher{}(t);
	hash = hash & ((1ULL<<(maxIdLen*7))-1);
	return Varint(hash);
}


struct VarLen {
private:
    std::array<std::byte, 8> buffer {std::byte{0x00}};
    std::size_t len {0};
    std::uint64_t val{0};

    static constexpr std::size_t calcOctetLen(std::uint64_t value) {
        for (int i{1}; i < 8; ++i) {
            if (value < ((1ULL<<(7*i))-2)) {
                return i;
            }
        }
        return 0;
    }

public:
    constexpr VarLen(std::uint64_t value, std::size_t minNumBytes=1) noexcept
        : len{std::max(minNumBytes, calcOctetLen(value))}
        , val{value}
    {
        for (std::size_t i{0}; i < len; ++i) {
            buffer[len-i-1] = std::byte{static_cast<unsigned char>((val >> (i*8)) & 0xff)};
        }
        std::byte head = static_cast<std::byte>(0x80 >> (len-1));
        buffer[0] |= head;
    }

    constexpr VarLen(std::byte const* buf, std::size_t buf_len) {
        if (buf_len == 0) {
            return;
        }
        std::byte head = *buf;
        if (head == std::byte{0x00}) { // this would be an invalid VarLen
            throw std::domain_error("invalid VarLen header");
        }
        len = 1;
        while (true) {
            if (head >> (8-len) == std::byte(0x01)) {
                break;
            }
            ++len;
        }
        if (buf_len < len) {
            throw std::length_error("not enough bytes to unpack VarLen");
        }
        std::copy(buf, buf+len, buffer.begin());
        // decode the VarLen
        val = std::to_integer<decltype(val)>(buffer[0]) & ((1 << (8-len))-1);
        for (std::size_t i{1}; i < len; ++i) {
            val = (val << 8) + std::to_integer<decltype(val)>(buf[i]);
        }
    }

    constexpr VarLen(VarLen const&) noexcept = default;
    constexpr VarLen& operator=(VarLen const&) noexcept = default;

    constexpr operator std::uint64_t() const noexcept {
        return val;
    }

    constexpr auto value() const noexcept {
        return val;
    }

    constexpr auto data() const noexcept {
        return buffer.data();
    }
    constexpr auto data() noexcept {
        return buffer.data();
    }
    
    constexpr auto size() const noexcept {
        return len;
    }

    constexpr auto begin() noexcept {
        return buffer.begin();
    }
    constexpr auto begin() const noexcept {
        return buffer.begin();
    }
    constexpr auto end() noexcept {
        return buffer.begin() + size();
    }
    constexpr auto end() const noexcept {
        return buffer.begin() + size();
    }
};

}
}
