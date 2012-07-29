//
//  StructMeta.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/23/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <type_traits>
#include <utility>

#ifndef Megacanvas_StructMeta_hpp
#define Megacanvas_StructMeta_hpp

//
// Define a macro MEGA_FIELDS_StructName to define a struct's fields,
// then use MEGA_STRUCT to define the struct:
//
// #define MEGA_FIELDS_StructName(x) \
//     x(foo, int) \
//     x(bar, char) \
//     x(bas, float)
// MEGA_STRUCT(StructName)
//
// You can use MEGA_FIELDS_StructName(FOO)
// to apply a macro FOO(name, type...) over all fields of the struct.
// Note that because the preprocessor does not understand template argument
// lists such as foo<bar,bas> as a single macro argument, the type macro
// argument must be variadic.
//

namespace Mega {
    template<typename T>
    using type = T;
    
#define MEGA_STRUCT_FIELD(name, ...) \
    ::Mega::type<__VA_ARGS__> name;

#define MEGA_STRUCT_APPLY_FIELD_METADATA(name, ...) \
    f(#name, sizeof(__VA_ARGS__), offsetof(self_type, name), \
      Trait<__VA_ARGS__>::value());

#define MEGA_STRUCT_APPLY_FIELD(name, ...) \
    f(#name, instance.name);

#define MEGA_STRUCT(NAME) \
    struct NAME { \
        MEGA_FIELDS_##NAME(MEGA_STRUCT_FIELD) \
        struct Q_struct_traits { \
            using self_type = NAME; \
            template<template<typename> class Trait, typename Function> \
            static void each_field_metadata(Function &&f) { \
                MEGA_FIELDS_##NAME(MEGA_STRUCT_APPLY_FIELD_METADATA) \
            } \
            template<typename Function> \
            static void each_field(NAME &instance, Function &&f) { \
                MEGA_FIELDS_##NAME(MEGA_STRUCT_APPLY_FIELD) \
            } \
            template<typename Function> \
            static void each_field(NAME const &instance, Function &&f) { \
                MEGA_FIELDS_##NAME(MEGA_STRUCT_APPLY_FIELD) \
            } \
            template<typename Function> \
            static void each_field(NAME &&instance, Function &&f) { \
                MEGA_FIELDS_##NAME(MEGA_STRUCT_APPLY_FIELD) \
            } \
        }; \
    };

    template<typename T>
    using struct_traits = typename T::Q_struct_traits;

    template<typename T, template<typename> class Trait, typename Function>
    void each_field_metadata(Function &&f) {
        struct_traits<T>::template each_field_metadata<Trait>(std::forward<Function>(f));
    }
    template<typename T, typename Function>
    void each_field(T &&instance, Function &&f) {
        struct_traits<typename std::remove_reference<T>::type>
        ::each_field(std::forward<T>(instance), std::forward<Function>(f));
    }
    
    template<typename F>
    struct Finally {
        F finalizer;
        
        ~Finally() { finalizer(); }
    };
    
    template<typename F>
    Finally<F> makeFinally(F &&x) { return {x}; }
    
#define __MEGA_CAT(a, b) a##b
#define _MEGA_CAT(a, b) __MEGA_CAT(a,b)
#define MEGA_FINALLY(block) auto _MEGA_CAT(Q_finally_, __LINE__) = makeFinally([&] block)
}

#endif
