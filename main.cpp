#include <iostream>

#include <iomanip>

#include "serializer/ebml/Serializer.h"
#include "serializer/ebml/Deserializer.h"

#include "serializer/yaml/Serializer.h"
#include "serializer/yaml/Deserializer.h"

#include "serializer/PolymorphConverter.h"

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

struct Base {
	virtual ~Base() = default;
	int baseVal {0};

	template<typename Serializer>
	void serialize(Serializer&& serializer) {
		serializer["baseVal"] % baseVal;
	}
};

struct DerivA : Base {
	int aVal {1};

	template<typename Serializer>
	void serialize(Serializer&& serializer) {
		Base::serialize(std::forward<Serializer>(serializer));
		serializer["aVal"] % aVal;
	}
};

struct DerivB : Base {
	int bVal {2};

	template<typename Serializer>
	void serialize(Serializer&& serializer) {
		Base::serialize(std::forward<Serializer>(serializer));
		serializer["bVal"] % bVal;
	}
};

serializer::Factory<Base, DerivA> facA{"DerivA"};
serializer::Factory<Base, DerivB> facB{"DerivB"};


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
	DerivA a;
	DerivB b;
	a.aVal = 3;
	b.bVal = 4;

	std::map<std::string, std::string> map {{"hallo", "welt"}, {"bla", "fasel"}};
	std::vector<int> vec {0, 1, -1, -2, -127, -128, -129, 0x0100, 0x800000};
	std::vector<Base*> polyVec {&a, &b};

	Serializable serializable{"Hallo Welt"};

	serializer["map"]         % map;
	serializer["vec"]         % vec;
	serializer["segment"]     % serializable;
	serializer["polymorphic"] % polyVec;

	serializer::ebml::Deserializer deserializer{serializer.getBuffer().data(), serializer.getBuffer().size()};

	decltype(map) deser_map;
	decltype(serializable) deser_serializable;
	decltype(vec) deser_vec;
	std::vector<std::unique_ptr<Base>> deser_polyVec;


	deserializer["segment"]     % deser_serializable;
	deserializer["map"]         % deser_map;
	deserializer["vec"]         % deser_vec;
	deserializer["polymorphic"] % deser_polyVec;

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
	DerivA a;
	DerivB b;
	a.aVal = 3;
	b.bVal = 4;

	std::map<std::string, std::string> map {{"hallo", "welt"}, {"bla", "fasel"}};
	std::vector<int> vec {0, 1, -1, -2, -127, -128, -129, 0x0100, 0x800000};
	
	std::vector<Base*> polyVec {&a, &b};
	
	Serializable serializable{"Hallo Welt"};

	serializer["map"]     % map;
	serializer["vec"]     % vec;
	serializer["segment"] % serializable;
	serializer["polymorphic"] % polyVec;

	serializer::yaml::Deserializer deserializer{serializer.getNode()};

	decltype(map) deser_map;
	decltype(serializable) deser_serializable;
	decltype(vec) deser_vec;
	std::vector<std::unique_ptr<Base>> deser_polyVec;

	deserializer["segment"]     % deser_serializable;
	deserializer["map"]         % deser_map;
	deserializer["vec"]         % deser_vec;
	deserializer["polymorphic"] % deser_polyVec;

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
