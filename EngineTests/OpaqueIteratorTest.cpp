//
//  OpaqueIteratorTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/28/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Engine/Util/OpaqueIterator.hpp"
#include <array>

namespace Mega { namespace test {
    class OpaqueIteratorTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(OpaqueIteratorTest);
        CPPUNIT_TEST(testArray2DRef);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() override
        {
        }

        void tearDown() override
        {
        }

        void testArray2DRef()
        {
            using namespace std;
            int aa[9] = {1,2,3,4,5,6,7,8,9};
            std::vector<int> a(begin(aa), end(aa));

            Array2DRef<int> a3(a, 3);
            CPPUNIT_ASSERT_EQUAL(std::size_t(3), a3.size());
            CPPUNIT_ASSERT_EQUAL(std::size_t(3), a3[0].size());
            CPPUNIT_ASSERT_EQUAL(1, a3[0][0]);
            CPPUNIT_ASSERT_EQUAL(3, a3[0][2]);
            CPPUNIT_ASSERT_EQUAL(7, a3[2][0]);
            CPPUNIT_ASSERT_EQUAL(9, a3[2][2]);

            int bb3[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
            auto i = begin(a3), ibegin = i, iend = end(a3);
            auto j = begin(bb3), jbegin = j, jend = end(bb3);
            for (; i != iend && j != jend; ++i, ++j)
                CPPUNIT_ASSERT(i->equals(*j));
            while (i-- != ibegin && j-- != jbegin)
                CPPUNIT_ASSERT(i->equals(*j));
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(OpaqueIteratorTest);
}}