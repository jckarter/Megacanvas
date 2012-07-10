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
        CPPUNIT_TEST(testLoadCanvasFailsWhenNonexistent);
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
            Owner<Canvas> canvasOwner = Canvas::create();
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tileLogSize() == 7);
            CPPUNIT_ASSERT(canvas.tileSize() == 128);
            CPPUNIT_ASSERT(canvas.tileArea() == 16384);
            CPPUNIT_ASSERT(canvas.tiles().size() == 0);
            CPPUNIT_ASSERT(canvas.layers().size() == 1);
            Layer layer = canvas.layers()[0];
            CPPUNIT_ASSERT(layer.parallax() == Vec(1., 1.));
            CPPUNIT_ASSERT(layer.priority() == 0);
        }

        void testLoadCanvasTest1()
        {
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("EngineTests/TestData/test1.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvasOwner);
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tileLogSize() == 7);
            CPPUNIT_ASSERT(canvas.tileSize() == 128);
            CPPUNIT_ASSERT(canvas.tileArea() == 128*128);
            CPPUNIT_ASSERT(canvas.tileByteSize() == 128*128*4);

            auto layers = canvas.layers();
            CPPUNIT_ASSERT(layers.size() == 2);

            CPPUNIT_ASSERT(layers[0].parallax() == Vec(1., 1.));
            CPPUNIT_ASSERT(layers[0].priority() == 0);

            CPPUNIT_ASSERT(layers[1].parallax() == Vec(0.5, 0.5));
            CPPUNIT_ASSERT(layers[1].priority() == 0);

            auto tiles = canvas.tiles();
            CPPUNIT_ASSERT(tiles.size() == 20);
            auto tile0 = tiles[0];
            auto tile1 = tiles[1];
            auto tile19 = tiles[19];
            CPPUNIT_ASSERT(tile0.size() == 128*128*4);
            CPPUNIT_ASSERT(tile1.size() == 128*128*4);
            CPPUNIT_ASSERT(tile0.end() <= tile1.begin() || tile1.end() <= tile0.begin());
            CPPUNIT_ASSERT(tile19.size() == 128*128*4);
        }

        void testLoadCanvasFailsWhenNonexistent()
        {
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("nonexistent.mega", &error); // must not exist
            CPPUNIT_ASSERT(!canvasOwner);
            CPPUNIT_ASSERT(error.find("unable to read file") != std::string::npos);
        }
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}