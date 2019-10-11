#pragma once
#include <cstddef>

namespace serializer::ebml::detail
{

template<typename T>
struct FNV1A_Hash {
	T hash = 2166136261U;
	T prime = 16777619U;

	constexpr FNV1A_Hash() {
		if constexpr (sizeof(T) == 4) {
			hash = 2166136261U;
			prime = 16777619U;
		}
		if constexpr (sizeof(T) == 8) {
			hash = 14695981039346656037UL;
			prime = 1099511628211UL;
		}
		static_assert(sizeof(T) == 4 or sizeof(T) == 8, "only 32 or 64 bit hashes are supported");
	}

	template<typename T2>
	constexpr T operator()(T2 const& buf) {
		for (auto const& e : buf) {
			hash ^= e;
			hash *= prime;
		}
		return hash;
	}
	constexpr T operator()() {
		return hash;
	}
};

using Hash = FNV1A_Hash<std::size_t>;

}
