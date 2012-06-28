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
        CPPUNIT_TEST(testNewCanvas);
        CPPUNIT_TEST(testLoadCanvasTest1);
        CPPUNIT_TEST_SUITE_END();
        
    public:
        void setUp() override
        {
        }
        
        void tearDown() override
        {
        }
        
        void testNewCanvas()
        {
            PrivOwner<Canvas> canvasOwner = Canvas::create();
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tileLogSize() == 7);
            CPPUNIT_ASSERT(canvas.tileSize() == 128);
            CPPUNIT_ASSERT(canvas.tileArea() == 16384);
            CPPUNIT_ASSERT(canvas.tileCount() == 0);
            CPPUNIT_ASSERT(canvas.layers().size() == 1);
            Layer layer = canvas.layers()[0];
            CPPUNIT_ASSERT(layer.parallax() == Vec(1., 1.));
            CPPUNIT_ASSERT(layer.priority() == 0);
        }
        
        void testLoadCanvasTest1()
        {
            PrivOwner<Canvas> canvasOwner = Canvas::load("TestData/test1.mega");
            CPPUNIT_ASSERT(canvasOwner);
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tileLogSize() == 7);
            CPPUNIT_ASSERT(canvas.tileSize() == 128);
            CPPUNIT_ASSERT(canvas.tileArea() == 16384);
            CPPUNIT_ASSERT(canvas.tileCount() == 20);
            auto tile0 = canvas.tile(0);
            auto tile1 = canvas.tile(1);
            auto tile19 = canvas.tile(19);
            CPPUNIT_ASSERT(tile0.size() == 16384*4);
            CPPUNIT_ASSERT(tile1.size() == 16384*4);
            CPPUNIT_ASSERT(tile0.end() <= tile1.begin() || tile1.end() <= tile0.begin());
            CPPUNIT_ASSERT(tile19.size() == 16384*4);
            
            auto layers = canvas.layers();
            CPPUNIT_ASSERT(layers.size() == 2);
            
            CPPUNIT_ASSERT(layers[0].parallax() == Vec(0.5, 0.5));
            CPPUNIT_ASSERT(layers[0].priority() == 0);
            
            CPPUNIT_ASSERT(layers[1].parallax() == Vec(1., 1.));
            CPPUNIT_ASSERT(layers[1].priority() == 0);
        }
    };
    
    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}