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
#include "Engine/Layer.hpp"

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
            PrivOwner<Canvas> canvasOwner = Canvas::create();
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tiles().empty());
            CPPUNIT_ASSERT(canvas.layers().size() == 1);
            Layer layer = canvas.layer(0);
            CPPUNIT_ASSERT(layer.parallax() == makeVec(1., 1.));
            CPPUNIT_ASSERT(layer.priority() == 0);
        }
    };
    
    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}