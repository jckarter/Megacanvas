//
//  TransformArrayRefTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <array>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Engine/Util/TransformArrayRef.hpp"

namespace Mega { namespace test {
    
    double times_eleven(int const &x) { return (double)(11*x); }
    
    class TransformArrayRefTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(TransformArrayRefTest);
        CPPUNIT_TEST(testIterator);
        CPPUNIT_TEST_SUITE_END();
        
    public:
        void setUp() override
        {
        }
        
        void tearDown() override
        {
        }
        
        void testIterator()
        {
            std::array<int, 3> foo{{1, 2, 3}};
            typedef TransformPointer<int const, double, times_eleven> Elevens;
            Elevens begin1{foo.begin()}, begin2{&foo[0]}, end{foo.end()};
            
            CPPUNIT_ASSERT(begin1 == begin2);
            CPPUNIT_ASSERT(!(begin1 == end));
            CPPUNIT_ASSERT(begin1 != end);
            CPPUNIT_ASSERT(!(begin1 != begin2));
            CPPUNIT_ASSERT(begin1 < end);
            CPPUNIT_ASSERT(!(begin1 < begin2));
            CPPUNIT_ASSERT(!(end < begin2));
            
            Elevens second = begin1;
            ++second;
            CPPUNIT_ASSERT(second == begin1 + 1);
            CPPUNIT_ASSERT(second - begin1 == 1);
            CPPUNIT_ASSERT(second - 1 == begin1);
            --second;
            CPPUNIT_ASSERT(second == begin1);
            
            CPPUNIT_ASSERT(*begin1 == 11.0);
            CPPUNIT_ASSERT(begin1[0] == 11.0);
            CPPUNIT_ASSERT(begin1[1] == 22.0);
            CPPUNIT_ASSERT(begin1[2] == 33.0);
        }
    };
    
    CPPUNIT_TEST_SUITE_REGISTRATION(TransformArrayRefTest);
}}