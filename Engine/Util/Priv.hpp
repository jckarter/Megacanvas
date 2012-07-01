//
//  Priv.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Priv_hpp
#define Megacanvas_Priv_hpp

#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include "Engine/Util/OpaqueIterator.hpp"

namespace Mega {
    template<typename T> struct Priv;
    
    template<typename T>
    struct HasPriv {
        Priv<T> *that;
        HasPriv() = default;
        /*implicit*/ HasPriv(Priv<T> &that) : that(&that) {}
        /*implicit*/ HasPriv(Priv<T> *that) : that(that) {}
        
        explicit operator bool() { return that != nullptr; }
        T *operator->() { return static_cast<T*>(this); }
    };
    
// inheriting constructors seem to be busted in xcode 4.3
#define MEGA_PRIV_CTORS(T) \
    T(Priv<T> &that) : HasPriv(that) {} \
    T(Priv<T> *that) : HasPriv(that) {} \
    T() = default;

    template<typename T>
    struct Owner {
    private:
        Priv<T> *that;
    public:
        explicit Owner() : that(nullptr) {}
        explicit Owner(Priv<T> *that) : that(that) {}
        Owner(Owner &&x) : that(x.that) { x.that = nullptr; }
        Owner &operator=(Owner &&x) { std::swap(that, x.that); return *this; }

        Owner(const Owner &) = delete;
        void operator=(const Owner &) = delete;
        
        ~Owner();

        T get() { return that; }
        T operator->() { return that; }
        Priv<T> &priv() { return *that; }
        explicit operator bool() { return that != nullptr; }
    };
    
    template<typename T, typename...AA>
    inline Owner<T> createOwner(AA&&...args) { return Owner<T>(new Priv<T>(std::forward<AA>(args)...)); }

    namespace {
        template<typename T>
        inline T fromOpaque(std::uint8_t *data, size_t size = 0) {
            return reinterpret_cast<Priv<T>*>(data);
        }
    }
        
    template<typename T>
    using PrivIterator = OpaqueIterator<std::uint8_t, T, fromOpaque<T>>;
    
    template<typename T>
    struct PrivArrayRef : OpaqueArrayRef<std::uint8_t, T, fromOpaque<T>> {
        typedef OpaqueArrayRef<std::uint8_t, T, fromOpaque<T>> super;
        
        PrivArrayRef() : super(nullptr, sizeof(Priv<T>), 0) {}
        PrivArrayRef(T oneElt) : super(*oneElt.that) {}
        PrivArrayRef(Priv<T> &oneElt) : super(oneElt) {}
        PrivArrayRef(std::vector<Priv<T>> &vec) : super(vec) {}
        // PrivArrayRef(Priv<T> (&)[N])
        // PrivArrayRef(SmallVectorImpl)
        // PrivArrayRef(Priv<T> *, Priv<T> *)
        // PrivArrayRef(Priv<T> *, size_t)
    };

#define MEGA_PRIV_DTOR(T) \
    template<> Owner<T>::~Owner() { delete that; }

#define MEGA_PRIV_GETTER(T, name, type) \
    type T::name() \
    { \
        return that->name; \
    }

#define MEGA_PRIV_SETTER(T, name, type) \
    void T::name(type value) \
    { \
        that->name = value; \
    }

#define MEGA_PRIV_GETTER_SETTER(T, name, type) \
    MEGA_PRIV_GETTER(T, name, type) \
    MEGA_PRIV_SETTER(T, name, type)

#ifndef NDEBUG
    namespace {
        struct _PrivTest : HasPriv<_PrivTest> { MEGA_PRIV_CTORS(_PrivTest); };
    }
    
    template<> struct Priv<_PrivTest> { Priv(int); ~Priv(); };
    
    namespace {
        static_assert(std::is_trivial<HasPriv<_PrivTest>>::value, "HasPriv should be trivial");
        static_assert(std::is_standard_layout<HasPriv<_PrivTest>>::value, "HasPriv should be standard layout");
        // xcode 4.3 believes it's not pod even though the above are all true...
        //static_assert(std::is_pod<HasPriv<_PrivTest>>::value, "HasPriv should be pod");
        static_assert(sizeof(HasPriv<_PrivTest>) == sizeof(void*), "HasPriv should be pointer-sized");
        
        static_assert(std::is_trivial<_PrivTest>::value, "HasPriv should be trivial");
        static_assert(std::is_standard_layout<_PrivTest>::value, "HasPriv should be standard layout");
        //static_assert(std::is_pod<_PrivTest>::value, "HasPriv should be pod");
        static_assert(sizeof(_PrivTest) == sizeof(void*), "HasPriv should be pointer-sized");
    }
#endif
}

#endif
