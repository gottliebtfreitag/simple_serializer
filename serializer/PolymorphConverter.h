#pragma once

#include <string>
#include <map>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

#include "yaml/Serializer.h"
#include "yaml/Deserializer.h"
#include "ebml/Serializer.h"
#include "ebml/Deserializer.h"

#include "demangle.h"

namespace serializer {


template<typename Base>
struct FactoryBase;

template<typename Base>
struct FactoryCollection {
private:
    std::map<std::string, FactoryBase<Base> const&> factories;

    using ReverseMappedType = typename std::map<std::string, FactoryBase<Base> const&>::value_type;
    std::map<std::type_index, ReverseMappedType const*> reverse_lokup;
    
public:
    static FactoryCollection& get() {
        static FactoryCollection instance{};
        return instance;
    }

    void addFactory(std::string const& name, FactoryBase<Base> const& factory) {
        auto res = factories.emplace(name, factory);
        if (res.second) {
            reverse_lokup[factory.getTypeInfo()] = &(*res.first);
        }
    }

    void removeFactory(FactoryBase<Base> const& factory) {
        auto it = std::find_if(begin(factories), end(factories), [&](auto const& a) { return &a.second==&factory; });
        if (it != factories.end() and &it->second == &factory) {
            auto other_it = std::find_if(begin(reverse_lokup), end(reverse_lokup), [&](auto const& a) { return a.second==&(*it); });
            reverse_lokup.erase(other_it);
            factories.erase(it);
        }
    }

    FactoryBase<Base> const* getFactory(std::string const& name) const {
        auto it = factories.find(name);
        if (it == factories.end()) {
            return nullptr;
        }
        return &it->second;
    }
    ReverseMappedType const& getFactory(std::type_info const& info) const {
        auto it = reverse_lokup.find(std::type_index{info});
        if (it == reverse_lokup.end()) {
            throw std::runtime_error("there is no factory for type: " + demangle(info));
        }
        return *it->second;
    }
};

template<typename Base>
struct FactoryBase {
    virtual std::unique_ptr<Base> build() const = 0;
    virtual std::type_info const& getTypeInfo() const = 0;

    virtual void forwardSerializer(yaml::Serializer& ser, Base& b) const = 0;
    virtual void forwardSerializer(yaml::Deserializer& ser, Base& b) const = 0;
    virtual void forwardSerializer(ebml::Serializer& ser, Base& b) const = 0;
    virtual void forwardSerializer(ebml::Deserializer& ser, Base& b) const = 0;
};

template<typename Base, typename Deriv>
struct Factory : FactoryBase<Base> {
    Factory(std::string const& name) {
        auto& collection = FactoryCollection<Base>::get();
        collection.addFactory(name, *this);
    }
    virtual ~Factory() {
        auto& collection = FactoryCollection<Base>::get();
        collection.removeFactory(*this);
    };

    std::unique_ptr<Base> build() const override {
        return std::make_unique<Deriv>();
    }

    std::type_info const& getTypeInfo() const override {
        return typeid(Deriv);
    }

    void forwardSerializer(yaml::Serializer& ser, Base& b) const override {
        ser % static_cast<Deriv&>(b);
    }
    void forwardSerializer(yaml::Deserializer& ser, Base& b) const override {
        ser % static_cast<Deriv&>(b);
    }
    void forwardSerializer(ebml::Serializer& ser, Base& b) const override {
        ser % static_cast<Deriv&>(b);
    }
    void forwardSerializer(ebml::Deserializer& ser, Base& b) const override {
        ser % static_cast<Deriv&>(b);
    }
};


template<typename Base>
struct Converter<std::unique_ptr<Base>, typename std::enable_if<std::is_polymorphic_v<Base>>::type> {
	using value_type = std::unique_ptr<Base>;
    
	template<typename Serializer>
	void serialize(Serializer& adapter, value_type& x) {
        auto const& collection = FactoryCollection<Base>::get();
		auto const& info = collection.getFactory(typeid(*x));

        adapter["specialization"] % info.first;
        auto&& subSer = adapter["content"];
        info.second.forwardSerializer(subSer, *x);
	}

	template<typename Deserializer>
	void deserialize(Deserializer& adapter, value_type& x) {
        auto const& collection = FactoryCollection<Base>::get();

        std::string name;
		adapter["specialization"] % name;

		auto factory = collection.getFactory(name);
        if (not factory) {
            // maybe its better to throw an exeption here... dunno
            return;
        }

        x = factory->build();
        auto&& subSer = adapter["content"];
        factory->forwardSerializer(subSer, *x);
	}
};

template<typename Base>
struct Converter<Base*, typename std::enable_if<std::is_polymorphic_v<Base>>::type> {
	using value_type = Base*;
    
	template<typename Serializer>
	void serialize(Serializer& adapter, value_type& x) {
        auto const& collection = FactoryCollection<Base>::get();
		auto const& info = collection.getFactory(typeid(*x));

        adapter["specialization"] % info.first;
        auto&& subSer = adapter["content"];
        info.second.forwardSerializer(subSer, *x);
	}

	template<typename Deserializer>
	void deserialize(Deserializer& adapter, value_type& x) {
        Converter<std::unique_ptr<Base>> conv;
        std::unique_ptr<Base> ptr;
        conv.deserialize(adapter, ptr);
        x = ptr.release();
	}
};

}


