#include <iostream>

#include <iomanip>

#include "serializer/ebml/Serializer.h"
#include "serializer/ebml/Deserializer.h"

#include "serializer/yaml/Serializer.h"
#include "serializer/yaml/Deserializer.h"

enum class bla {
	foo,
	bar
};

struct Serializable {
	std::string content;

	bool operator!=(Serializable const& rhs) const {
		return content != rhs.content;
	}

	template<typename Serializer>
	void serialize(Serializer&& serializer) {
		serializer["content"] % content;
	}
};

void prettyPrintBuffer(serializer::ebml::Buffer const& buffer) {
	int counter {0};
	for (auto i : buffer) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i) << " ";
		if (++counter % 8 == 0) {
			std::cout << "  ";
		}
	}
	std::cout << "\n";

	counter = 0;
	for (auto i_ : buffer) {
		std::uint8_t i = static_cast<std::uint8_t>(i_);
		if (::isprint(i)) {
			std::cout << " " << i << " ";
		} else {
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(i) << " ";
		}
		if (++counter % 8 == 0) {
			std::cout << "  ";
		}
	}
	std::cout << "\n";
}

void test_ebml() {
	serializer::ebml::Serializer serializer;

	std::map<std::string, std::string> map {{"hallo", "welt"}, {"bla", "fasel"}};
	std::vector<int> vec {1, 0x0100, 0x800000};

	Serializable serializable{"Hallo Welt"};

	serializer["map"]     % map;
	serializer["vec"]     % vec;
	serializer["segment"] % serializable;

	serializer::ebml::Deserializer deserializer{serializer.getBuffer().data(), serializer.getBuffer().size()};

	decltype(map) deser_map;
	decltype(serializable) deser_serializable;
	decltype(vec) deser_vec;

	deserializer["segment"] % deser_serializable;
	deserializer["map"]     % deser_map;
	deserializer["vec"]     % deser_vec;

	if (deser_serializable != serializable) {
		std::cerr << "something went horribly wrong! \n";
	}
	if (deser_map != map) {
		std::cerr << "something went horribly wrong! \n";
	}

	for (auto const& [key, value] : deser_map) {
		std::cout << key << ": " << value << "\n";
	}
	std::cout << deser_serializable.content << "\n";
	for (auto const& v : deser_vec) {
		std::cout << v << " ";
	}
	std::cout << "\n\n";

	prettyPrintBuffer(serializer.getBuffer());

	std::cout << "\n\n" << std::endl;
}

void test_yaml() {
	serializer::yaml::Serializer serializer;

	std::map<std::string, std::string> map {{"hallo", "welt"}, {"bla", "fasel"}};
	std::vector<int> vec {1, 0x0100, 0x800000};

	Serializable serializable{"Hallo Welt"};

	serializer["map"]     % map;
	serializer["vec"]     % vec;
	serializer["segment"] % serializable;

	serializer::yaml::Deserializer deserializer{serializer.getNode()};

	decltype(map) deser_map;
	decltype(serializable) deser_serializable;
	decltype(vec) deser_vec;

	deserializer["segment"] % deser_serializable;
	deserializer["map"]     % deser_map;
	deserializer["vec"]     % deser_vec;

	if (deser_serializable != serializable) {
		std::cerr << "something went horribly wrong! \n";
	}
	if (deser_map != map) {
		std::cerr << "something went horribly wrong! \n";
	}

	for (auto const& [key, value] : deser_map) {
		std::cout << key << ": " << value << "\n";
	}
	std::cout << deser_serializable.content << "\n";
	for (auto const& v : deser_vec) {
		std::cout << v << " ";
	}

	std::cout << "\n\n" << serializer.getNode() << "\n\n" << std::endl;
}

int main() {
	test_ebml();
	test_yaml();
}
