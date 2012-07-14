//
//  NamedTupleTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/1/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Engine/Util/NamedTuple.hpp"
#include <llvm/ADT/StringRef.h>

namespace Mega { namespace test {
    template<typename T> struct test_trait;
    template<> struct test_trait<int> { static int value() { return 1; } };
    template<> struct test_trait<float> { static int value() { return 2; } };
    template<> struct test_trait<char> { static int value() { return 3; } };

    class NamedTupleTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(NamedTupleTest);
        CPPUNIT_TEST(testNamedTupleConstruction);
        CPPUNIT_TEST(testNamedTupleOffsetOf);
        CPPUNIT_TEST(testEachField);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() override
        {
        }

        void tearDown() override
        {
        }

        MEGA_FIELD(foo)
        MEGA_FIELD(bar)
        MEGA_FIELD(bas)
        using SomeTuple = NamedTuple<foo<int>, bar<char>, bas<float>>;

        void testNamedTupleConstruction()
        {
            SomeTuple t(1, '2', 3.0f);
            CPPUNIT_ASSERT_EQUAL(1, t.foo);
            CPPUNIT_ASSERT_EQUAL('2', t.bar);
            CPPUNIT_ASSERT_EQUAL(3.0f, t.bas);
        }

        void testNamedTupleOffsetOf()
        {
            std::size_t foo_offset = SomeTuple::offset_of<foo>();
            std::size_t bar_offset = SomeTuple::offset_of<bar>();
            std::size_t bas_offset = SomeTuple::offset_of<bas>();

            SomeTuple t(1, '2', 3.0f);

            int *fooPtr = reinterpret_cast<int*>(reinterpret_cast<char*>(&t) + foo_offset);
            CPPUNIT_ASSERT_EQUAL(1, *fooPtr);
            char *barPtr = reinterpret_cast<char*>(&t) + bar_offset;
            CPPUNIT_ASSERT_EQUAL('2', *barPtr);
            float *basPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(&t) + bas_offset);
            CPPUNIT_ASSERT_EQUAL(3.0f, *basPtr);
        }

        static void assert_field(llvm::StringRef name, std::size_t offset, std::size_t size, int type) {
            if (name == "foo") {
                CPPUNIT_ASSERT_EQUAL(sizeof(int), size);
                CPPUNIT_ASSERT_EQUAL(SomeTuple::offset_of<foo>(), offset);
                CPPUNIT_ASSERT_EQUAL(test_trait<int>::value(), type);
            } else if (name == "bar") {
                CPPUNIT_ASSERT_EQUAL(sizeof(char), size);
                CPPUNIT_ASSERT_EQUAL(SomeTuple::offset_of<bar>(), offset);
                CPPUNIT_ASSERT_EQUAL(test_trait<char>::value(), type);
            } else if (name == "bas") {
                CPPUNIT_ASSERT_EQUAL(sizeof(float), size);
                CPPUNIT_ASSERT_EQUAL(SomeTuple::offset_of<bas>(), offset);
                CPPUNIT_ASSERT_EQUAL(test_trait<float>::value(), type);
            }
        }

        void testEachField()
        {
            SomeTuple::eachField<test_trait>(assert_field);
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(NamedTupleTest);
}}