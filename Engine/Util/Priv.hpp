//
//  Priv.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Priv_hpp
#define Megacanvas_Priv_hpp

#include <memory>
#include <utility>

namespace Mega {
    template<typename T> struct Priv;
    
    template<typename T>
    struct HasPriv {
        Priv<T> *that;
        HasPriv() = default;
        /*implicit*/ HasPriv(Priv<T> &that) : that(&that) {}
        /*implicit*/ HasPriv(Priv<T> *that) : that(that) {}
        
        explicit operator bool() { return that != nullptr; }
    };

// inheriting constructors seem to be busted in xcode 4.3
#define MEGA_PRIV_CTORS(T) \
    T(Priv<T> &that) : HasPriv(that) {} \
    T(Priv<T> *that) : HasPriv(that) {} \
    T() = default;

    template<typename T>
    struct PrivOwner {
    private:
        Priv<T> *that;
    public:
        PrivOwner(PrivOwner &&x) : that(x.that) { x.that = nullptr; }
        PrivOwner &operator=(PrivOwner &&x) { std::swap(that, x.that); }

        PrivOwner(const PrivOwner &) = delete;
        void operator=(const PrivOwner &) = delete;
        
        template<typename...AA>
        PrivOwner(AA&&...args) : that(new Priv<T>(std::forward<AA>(args)...)) {}
        ~PrivOwner();
        
        T get() { return that; }
        explicit operator bool() { return that != nullptr; }
    };

#define MEGA_PRIV_DTOR(T) \
    template<> PrivOwner<T>::~PrivOwner() { delete that; }

#ifndef NDEBUG
    struct _PrevTest : HasPriv<_PrevTest> { MEGA_PRIV_CTORS(_PrevTest); };
    template<> struct Priv<_PrevTest> { Priv(int); ~Priv(); };
    
    static_assert(std::is_trivially_default_constructible<HasPriv<_PrevTest>>::value, "HasPriv should be trivially default constructible");
    static_assert(std::is_trivially_copy_constructible<HasPriv<_PrevTest>>::value, "HasPriv should be trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<HasPriv<_PrevTest>>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivially_destructible<HasPriv<_PrevTest>>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivial<HasPriv<_PrevTest>>::value, "HasPriv should be trivial");
    static_assert(std::is_standard_layout<HasPriv<_PrevTest>>::value, "HasPriv should be standard layout");
    // xcode 4.3 believes it's not pod even though the above are all true...
    //static_assert(std::is_pod<HasPriv<_PrevTest>>::value, "HasPriv should be pod");
    static_assert(sizeof(HasPriv<_PrevTest>) == sizeof(void*), "HasPriv should be pointer-sized");

    static_assert(std::is_trivially_default_constructible<_PrevTest>::value, "HasPriv should be trivially constructible");
    static_assert(std::is_trivially_copy_constructible<_PrevTest>::value, "HasPriv should be trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<_PrevTest>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivially_destructible<_PrevTest>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivial<_PrevTest>::value, "HasPriv should be trivial");
    static_assert(std::is_standard_layout<_PrevTest>::value, "HasPriv should be standard layout");
    //static_assert(std::is_pod<_PrevTest>::value, "HasPriv should be pod");
    static_assert(sizeof(_PrevTest) == sizeof(void*), "HasPriv should be pointer-sized");
#endif
}

#endif
