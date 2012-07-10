//
//  NamedTuple.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/1/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_NamedTuple_hpp
#define Megacanvas_NamedTuple_hpp

#include <cstdint>
#include <type_traits>

namespace Mega {
#define MEGA_FIELD(fieldname) \
    template<typename __T> \
    struct fieldname { \
        __T fieldname; \
        using type = __T; \
        constexpr static char const *name() { return #fieldname; } \
        __T &value() & { return this->fieldname; } \
        constexpr __T const &value() const & { return this->fieldname; } \
        constexpr __T &&value() && { return this->fieldname; } \
    };

    namespace {
        template<template<typename T> class FieldTemplate, typename...FieldInstances>
        struct find_field;

        template<template<typename T> class FieldTemplate, typename T, typename...FieldInstances>
        struct find_field<FieldTemplate, FieldTemplate<T>, FieldInstances...> {
            using field = FieldTemplate<T>;
            using type = T;
        };
        template<template<typename T> class FieldTemplate, typename FieldInstance, typename...FieldInstances>
        struct find_field<FieldTemplate, FieldInstance, FieldInstances...> : find_field<FieldTemplate, FieldInstances...> {};
    }

    template<typename...Fields>
    struct NamedTuple : Fields... {
        using tuple_type = NamedTuple;

        NamedTuple() = default;
        NamedTuple(NamedTuple const &) = default;
        template<typename...TT>
        constexpr NamedTuple(TT &&...x) : Fields{static_cast<TT&&>(x)}... {} // can't use std::forward because it isn't constexpr

        // FIXME relies on undefined behavior, can't be constexpr
        template<typename Field>
        /*constexpr*/ static uintptr_t offset_of() { return reinterpret_cast<uintptr_t>(&static_cast<NamedTuple*>(nullptr)->Field::value()); }

        template<template<typename T> class Field>
        /*constexpr*/ static uintptr_t offset_of() { return reinterpret_cast<uintptr_t>(&static_cast<NamedTuple*>(nullptr)->find_field<Field, Fields...>::field::value()); }

        template<typename Field>
        constexpr static uintptr_t size_of() { return sizeof(typename Field::type); }

        template<template<typename T> class Field>
        constexpr static uintptr_t size_of() { return sizeof(typename find_field<Field, Fields...>::type); }

        template<template<typename T> class Field>
        using type_of = typename find_field<Field, Fields...>::type;

        template<template<typename T> class Trait, typename Function>
        static void eachField(Function &&f)
        {
            char __attribute__((unused)) discard[] = {(f(Fields::name(), offset_of<Fields>(), size_of<Fields>(), Trait<typename Fields::type>::value()), '\0')...};
        }
        
        template<typename Function>
        void eachInstanceField(Function &&f)
        {
            char __attribute__((unused)) discard[] = {(f(Fields::name(), this->Fields::value()), '\0')...};
        }
    };

#ifndef NDEBUG
    namespace test {
        MEGA_FIELD(_x)
        MEGA_FIELD(_y)
        MEGA_FIELD(_z)
        using _TestTuple = NamedTuple<_x<int>, _y<float>, _z<char>>;
        static_assert(std::is_trivial<_TestTuple>::value, "tuple should be trivial");
        static_assert(std::is_same<decltype(_TestTuple()._x), int>::value, "decltype of x should be int");
        static_assert(std::is_same<decltype(_TestTuple()._y), float>::value, "decltype of y should be float");
        static_assert(std::is_same<decltype(_TestTuple()._z), char>::value, "decltype of z should be char");
        static_assert(std::is_same<_TestTuple::type_of<_x>, int>::value, "type_of x should be int");
        static_assert(std::is_same<_TestTuple::type_of<_y>, float>::value, "type_of y should be float");
        static_assert(std::is_same<_TestTuple::type_of<_z>, char>::value, "type_of z should be char");
        // xcode 4.3 isn't quite there with literal types and ilists
#if __has_feature(cxx_constexpr)
        static_assert(_TestTuple::size_of<_x>() == sizeof(int), "size_of<x> should be sizeof(int)");
        static_assert(_TestTuple::size_of<_y>() == sizeof(float), "size_of<y> should be sizeof(float)");
        static_assert(_TestTuple::size_of<_z>() == sizeof(char), "size_of<z> should be sizeof(char)");
//        static_assert(_TestTuple::offset_of<_x>() != _TestTuple::offset_of<_y>()
//                      && _TestTuple::offset_of<_y>() != _TestTuple::offset_of<_z>()
//                      && _TestTuple::offset_of<_z>() != _TestTuple::offset_of<_x>(), "fields should have different offsets");
        static_assert(_TestTuple(1, 2.0f, '3')._x == 1, "tuple should be constexpr constructible; _x should be constexpr accessible");
        static_assert(_TestTuple{1, 2.0f, '3'}._z == '3', "tuple should be constexpr ilist constructible; _z should be constexpr accessible");
#endif
    }
#endif
    
    namespace fields {
        MEGA_FIELD(center)
        MEGA_FIELD(invTileCount)
        MEGA_FIELD(invTileTrimSize)
        MEGA_FIELD(layerParallax)
        MEGA_FIELD(layerOrigin)
        MEGA_FIELD(mappingTexture)
        MEGA_FIELD(mappingTextureScale)
        MEGA_FIELD(padding1)
        MEGA_FIELD(padding2)
        MEGA_FIELD(tilesTexture)
        MEGA_FIELD(tileCoord)
        MEGA_FIELD(tileCorner)
        MEGA_FIELD(tileCount)
        MEGA_FIELD(tileTexLo)
        MEGA_FIELD(tileTexSize)
        MEGA_FIELD(tileTrimSize)
        MEGA_FIELD(tilePhase)
        MEGA_FIELD(viewportScale)
        MEGA_FIELD(zoom)
    }
    using namespace fields;
}

#endif
