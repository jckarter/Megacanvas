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
#include "GLTest.hpp"

namespace Mega { namespace test {
    class CanvasTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(CanvasTest);
        CPPUNIT_TEST(testNewCanvas);
        CPPUNIT_TEST(testLoadCanvasTest1);
        CPPUNIT_TEST(testLoadCanvasFailsWhenNonexistent);
        CPPUNIT_TEST(testLayerGetSegment);
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
        
        void testLayerGetSegment()
        {
            using namespace std;
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("EngineTests/TestData/test2.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvasOwner);
            Canvas canvas = canvasOwner.get();
            
            // layer 0: depth 1, tiles 1 thru 4
            Layer layer0 = canvas.layers()[0], layer1 = canvas.layers()[1], layer2 = canvas.layers()[2];
            Layer::tile_t oneTile;
            Layer::tile_t fourTiles[4];
            Layer::tile_t sixteenTiles[16];
            Layer::tile_t sixtyfourTiles[64];
            
            layer0.getSegment(-1, -1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(1), oneTile);
            layer0.getSegment(0, -1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(2), oneTile);
            layer0.getSegment(-1, 0, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(3), oneTile);
            layer0.getSegment(0, 0, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(4), oneTile);
            
            layer0.getSegment(-2, -1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer0.getSegment(1, -1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer0.getSegment(-1, -2, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer0.getSegment(-1, 1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            
            layer0.getSegment(-1, -1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 0, 0, 1);
            layer0.getSegment(0, -1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 0, 2, 0);
            layer0.getSegment(-1, 0, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 3, 0, 0);
            layer0.getSegment(0, 0, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 4, 0, 0, 0);
            
            layer0.getSegment(-1, -1, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 1);
            layer0.getSegment(0, -1, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             2, 0, 0, 0);
            layer0.getSegment(-1, 0, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles,
                                             0, 0, 0, 3,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0);
            layer0.getSegment(0, 0, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles,
                                             4, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0);

            layer0.getSegment(-2, -2, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 0, 0, 0);
            
            // layer 1: depth 2, tiles 5 thru 20
            //  5  6  9 10
            //  7  8 11 12
            //      *
            // 13 14 17 18
            // 15 16 19 20
            layer1.getSegment(-2, 1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(15), oneTile);
            layer1.getSegment(-1, -1, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(8), oneTile);
            layer1.getSegment(0, -2, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(9), oneTile);
            layer1.getSegment(1, 0, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(18), oneTile);

            layer1.getSegment(-3, 0, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer1.getSegment(2, 0, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer1.getSegment(0, -3, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);
            layer1.getSegment(0, 2, &oneTile, 1);
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), oneTile);

            layer1.getSegment(-1, -1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 5, 6, 7, 8);
            layer1.getSegment(0, -1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 9, 10, 11, 12);
            layer1.getSegment(-1, 0, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 13, 14, 15, 16);
            layer1.getSegment(0, 0, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 17, 18, 19, 20);
            
            layer1.getSegment(-2, -2, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 0, 0, 0);
            layer1.getSegment(1, 1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 0, 0, 0, 0);
            
            layer1.getSegment(-1, -1, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 5, 6,
                                             0, 0, 7, 8);
            layer1.getSegment(0, -1, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             0,  0, 0, 0,
                                             0,  0, 0, 0,
                                             9, 10, 0, 0,
                                             11, 12, 0, 0);
            layer1.getSegment(-1, 0, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             0,  0, 13, 14,
                                             0,  0, 15, 16,
                                             0,  0,  0,  0,
                                             0,  0,  0,  0);
            layer1.getSegment(0, 0, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             17, 18, 0, 0,
                                             19, 20, 0, 0,
                                             0,  0, 0, 0,
                                             0,  0, 0, 0);
            
            // layer 2: depth 3, tiles 21 thru 84
            // 21-24 25-28   37-40 41-44
            // 29-32 33-36   45-48 49-52
            //             *
            // 53-56 57-60   69-72 73-76
            // 61-64 65-68   77-80 81-84
            layer2.getSegment(-1, -1, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 33, 34, 35, 36);
            layer2.getSegment(1, 0, fourTiles, 2);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(fourTiles, 73, 74, 75, 76);
            
            layer2.getSegment(-1, -1, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             21, 22, 25, 26,
                                             23, 24, 27, 28,
                                             29, 30, 33, 34,
                                             31, 32, 35, 36);
            layer2.getSegment(0, 0, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             69, 70, 73, 74,
                                             71, 72, 75, 76,
                                             77, 78, 81, 82,
                                             79, 80, 83, 84);
            
            layer2.getSegment(2, 2, sixteenTiles, 4);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixteenTiles, 
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0);
            
            layer2.getSegment(-1, -1, sixtyfourTiles, 8);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixtyfourTiles, 
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 21, 22, 25, 26,
                                             0, 0, 0, 0, 23, 24, 27, 28,
                                             0, 0, 0, 0, 29, 30, 33, 34,
                                             0, 0, 0, 0, 31, 32, 35, 36);
            layer2.getSegment(0, 0, sixtyfourTiles, 8);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(sixtyfourTiles, 
                                             69, 70, 73, 74, 0, 0, 0, 0,
                                             71, 72, 75, 76, 0, 0, 0, 0,
                                             77, 78, 81, 82, 0, 0, 0, 0,
                                             79, 80, 83, 84, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0);
        }
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}