#pragma once

#include <cxxabi.h>
#include <exception>
#include <iostream>


namespace serializer {

std::string demangle(std::type_info const& ti);

template<typename T>
std::string demangle(T const& t) {
	return demangle(typeid(t));
}

template<typename T>
std::string demangle() {
	return demangle(typeid(T));
}

}