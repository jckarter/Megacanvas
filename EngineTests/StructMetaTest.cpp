//
//  StructMetaTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/23/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Engine/Util/StructMeta.hpp"
#include <array>
#include <vector>

namespace Mega { namespace test {
    enum TypeToken { Int, Float, ArrayInt4 };
    
    template<typename> struct TypeTokenTraits;
    template<> struct TypeTokenTraits<int> {
        static TypeToken value() { return Int; }
    };
    template<> struct TypeTokenTraits<float> {
        static TypeToken value() { return Float; }
    };
    template<> struct TypeTokenTraits<std::array<int,4>> {
        static TypeToken value() { return ArrayInt4; }
    };
    
    std::ostream& operator<<(std::ostream& os, std::tuple<char const*,int,int,TypeToken> const &t)
    {
        return os << std::get<0>(t)
        << ", " << std::get<1>(t)
        << ", " << std::get<2>(t)
        << ", " << std::get<3>(t);
    }
    
    class StructMetaTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(StructMetaTest);
        CPPUNIT_TEST(testEachFieldMetadata);
        CPPUNIT_TEST_SUITE_END();
        
    public:
        void setUp() override
        {
        }
        
        void tearDown() override
        {
        }
        
        void testEachFieldMetadata()
        {
            using namespace std;
#define MEGA_FIELDS_Foo(x) \
    x(foo, int) \
    x(bar, float) \
    x(bas, array<int, 4>)
            MEGA_STRUCT(Foo)
            vector<tuple<char const *, int, int, TypeToken>> info;
            
            struct pusher {
                decltype(info) &i;
                pusher(decltype(info) &i) : i(i) {}
                void operator()(char const * name, int size, int offset, TypeToken token) {
                    i.push_back(make_tuple(name, size, offset, token));
                }
            };
            
            each_field_metadata<Foo, TypeTokenTraits>(pusher(info));
            
            CPPUNIT_ASSERT_EQUAL(typename type<decltype(info)>::size_type(3), info.size());
            CPPUNIT_ASSERT_EQUAL(make_tuple("foo", int(sizeof(int)), int(offsetof(Foo, foo)), Int),
                                 info[0]);
            CPPUNIT_ASSERT_EQUAL(make_tuple("bar", int(sizeof(float)), int(offsetof(Foo, bar)), Float),
                                 info[1]);
            CPPUNIT_ASSERT_EQUAL(make_tuple("bas", int(sizeof(array<int,4>)), int(offsetof(Foo, bas)), ArrayInt4),
                                 info[2]);
        }
        
        void testEachField()
        {
#define MEGA_FIELDS_Bar(x) \
    x(foo, int) \
    x(bar, float) \
    x(bas, unsigned char)
            MEGA_STRUCT(Bar)
            
            struct fill {
                int x = 0;
                template<typename T>
                void operator()(T &y) {
                    y = x++;
                }
            };
            
            Bar bar;
            
            each_field(bar, fill());
            CPPUNIT_ASSERT_EQUAL(int(0), bar.foo);
            CPPUNIT_ASSERT_EQUAL(float(1.0f), bar.bar);
            CPPUNIT_ASSERT_EQUAL((unsigned char)2, bar.bas);
        }
    };    
    CPPUNIT_TEST_SUITE_REGISTRATION(StructMetaTest);
}}