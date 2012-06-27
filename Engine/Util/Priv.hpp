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
        Priv<T> *getPriv() { return that; }
        explicit operator bool() { return that != nullptr; }
    };

    template<typename T>
    inline Priv<T> *privFromOpaque(std::uint8_t *data) {
        return reinterpret_cast<Priv<T>*>(data);
    }
    template<typename T>
    inline std::uint8_t *privToOpaque(Priv<T> *data) {
        return reinterpret_cast<std::uint8_t*>(data);
    }
        
    template<typename T>
    struct PrivIterator {
        std::uint8_t *ptr;
        size_t isize;
        
        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        typedef std::random_access_iterator_tag iterator_category;
        
        PrivIterator() = default;
        PrivIterator(std::uint8_t *ptr, size_t size)
        : ptr(ptr), isize(size)
        {}

#define MEGA_COMPARE_OPERATOR(op) \
    bool operator op(PrivIterator const &p) const { return ptr op p.ptr; }
        
        MEGA_COMPARE_OPERATOR(==)
        MEGA_COMPARE_OPERATOR(!=)
        MEGA_COMPARE_OPERATOR(<)
        MEGA_COMPARE_OPERATOR(<=)
        MEGA_COMPARE_OPERATOR(>=)
        MEGA_COMPARE_OPERATOR(>)
#undef MEGA_COMPARE_OPERATOR
        
#define MEGA_INCDEC_OPERATOR(op, arop) \
    PrivIterator &operator op() { ptr arop isize; return *this; } \
    PrivIterator operator op(int) { \
        PrivIterator r = ptr; \
        ptr arop isize; \
        return r; \
    }
        
        MEGA_INCDEC_OPERATOR(--, -)
        MEGA_INCDEC_OPERATOR(++, +)
#undef MEGA_INCDEC_OPERATOR
        
#define MEGA_MATH_OPERATOR(op) \
    PrivIterator operator op(std::ptrdiff_t dist) const { \
        return PrivIterator<T>(ptr op (dist*isize), isize); \
    }
        
        MEGA_MATH_OPERATOR(+)
        MEGA_MATH_OPERATOR(-)
#undef MEGA_MATH_OPERATOR
        
#define MEGA_COMPOUND_OPERATOR(op) \
    PrivIterator &operator op(std::ptrdiff_t dist) { \
        ptr op (dist*isize); \
        return *this; \
    }
        
        MEGA_COMPOUND_OPERATOR(+=)
        MEGA_COMPOUND_OPERATOR(-=)
#undef MEGA_COMPOUND_OPERATOR
        
        std::ptrdiff_t operator-(PrivIterator const &p) const { return ptr - p.ptr; }
        
        T operator*()  const { return privFromOpaque<T>(ptr); }
        T operator->() const { return privFromOpaque<T>(ptr); }
        T operator[](std::ptrdiff_t i) const {
            return privFromOpaque<T>(ptr + i*isize);
        }
    };
    
    template<typename T>
    struct PrivArrayRef {
        std::uint8_t *data;
        size_t isize, length;
        
        PrivArrayRef() : data(nullptr), isize(0), length(0) {}
        PrivArrayRef(T oneElt)
        : data(privToOpaque(oneElt.that)), isize(sizeof(Priv<T>)), length(1)
        {}
        PrivArrayRef(Priv<T> &oneElt)
        : data(privToOpaque(&oneElt)), isize(sizeof(Priv<T>)), length(1)
        {}
        PrivArrayRef(std::vector<Priv<T>> &vec)
        : data(privToOpaque(&vec[0])), isize(sizeof(Priv<T>)), length(vec.size())
        {}
        // PrivArrayRef(Priv<T> (&)[N])
        // PrivArrayRef(SmallVectorImpl)
        // PrivArrayRef(Priv<T> *, Priv<T> *)
        // PrivArrayRef(Priv<T> *, size_t)
        
        typedef PrivIterator<T> iterator;
        typedef size_t size_type;
        
        iterator begin() const { return iterator(data, isize); }
        iterator end() const { return iterator(data + isize*length, isize); }
        bool empty() const { return length == 0; }
        size_t size() const { return length; }
        
        T front() const {
            assert(!empty());
            return privFromOpaque<T>(data);
        }
        
        T back() const {
            assert(!empty());
            return privFromOpaque<T>(data + isize*(length-1));
        }
        
        T operator[](size_t i) const {
            assert(i < length);
            return privFromOpaque<T>(data + isize*i);
        }
        
        // equals
        // slice
    };

#define MEGA_PRIV_DTOR(T) \
    template<> PrivOwner<T>::~PrivOwner() { delete that; }

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
    struct _PrivTest : HasPriv<_PrivTest> { MEGA_PRIV_CTORS(_PrivTest); };
    template<> struct Priv<_PrivTest> { Priv(int); ~Priv(); };
    
    static_assert(std::is_trivially_default_constructible<HasPriv<_PrivTest>>::value, "HasPriv should be trivially default constructible");
    static_assert(std::is_trivially_copy_constructible<HasPriv<_PrivTest>>::value, "HasPriv should be trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<HasPriv<_PrivTest>>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivially_destructible<HasPriv<_PrivTest>>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivial<HasPriv<_PrivTest>>::value, "HasPriv should be trivial");
    static_assert(std::is_standard_layout<HasPriv<_PrivTest>>::value, "HasPriv should be standard layout");
    // xcode 4.3 believes it's not pod even though the above are all true...
    //static_assert(std::is_pod<HasPriv<_PrivTest>>::value, "HasPriv should be pod");
    static_assert(sizeof(HasPriv<_PrivTest>) == sizeof(void*), "HasPriv should be pointer-sized");

    static_assert(std::is_trivially_default_constructible<_PrivTest>::value, "HasPriv should be trivially constructible");
    static_assert(std::is_trivially_copy_constructible<_PrivTest>::value, "HasPriv should be trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<_PrivTest>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivially_destructible<_PrivTest>::value, "HasPriv should be trivially copy assignable");
    static_assert(std::is_trivial<_PrivTest>::value, "HasPriv should be trivial");
    static_assert(std::is_standard_layout<_PrivTest>::value, "HasPriv should be standard layout");
    //static_assert(std::is_pod<_PrivTest>::value, "HasPriv should be pod");
    static_assert(sizeof(_PrivTest) == sizeof(void*), "HasPriv should be pointer-sized");
#endif
}

#endif
