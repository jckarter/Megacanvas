//
//  CanvasTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <utility>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "Engine/Canvas.hpp"

namespace Mega { namespace test {
    class CanvasTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(CanvasTest);
        CPPUNIT_TEST(testInitialization);
        CPPUNIT_TEST_SUITE_END();
        
    public:
        void setUp() override
        {
        }
        
        void tearDown() override
        {
        }
        
        void testInitialization()
        {
            Mega::Canvas canvas;
            CPPUNIT_ASSERT(canvas.layers().empty());
        }
    };
    
    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}