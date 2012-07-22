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
        CPPUNIT_TEST(testLayerGetSegmentEmptyLayer);
        CPPUNIT_TEST(testVerifyTiles);
        CPPUNIT_TEST(testLoadTile);
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
            CPPUNIT_ASSERT(canvas.tileCount() == 0);
            CPPUNIT_ASSERT(canvas.layers().size() == 1);
            Layer layer = canvas.layers()[0];
            CPPUNIT_ASSERT(layer.parallax() == Vec(1., 1.));
        }

        void testLoadCanvasTest1()
        {
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("EngineTests/TestData/Test1.mega", &error);
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

            CPPUNIT_ASSERT(layers[1].parallax() == Vec(0.5, 0.5));

            CPPUNIT_ASSERT(canvas.tileCount() == 20);
        }

        void testLoadCanvasFailsWhenNonexistent()
        {
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("nonexistent.mega", &error); // must not exist
            CPPUNIT_ASSERT(!canvasOwner);
            CPPUNIT_ASSERT(error.find("unable to read file") != std::string::npos);
        }
        
#define _MEGA_CPPUNIT_ASSERT_SEGMENT(actual, expectedOffset, ...) \
    do { \
        auto Q_seg = (actual); \
        CPPUNIT_ASSERT_EQUAL(std::size_t(expectedOffset), Q_seg.offset); \
        MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(Q_seg.tiles, __VA_ARGS__); \
    } while (false)
#define _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(actual) \
    do { \
        auto Q_seg = (actual); \
        CPPUNIT_ASSERT_EQUAL(std::size_t(0), Q_seg.offset); \
        CPPUNIT_ASSERT_EQUAL(std::size_t(0), Q_seg.tiles.size()); \
    } while (false)

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

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(1, -1, -1), 0, 1);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(1,  0, -1), 0, 2);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(1, -1,  0), 0, 3);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(1,  0,  0), 0, 4);
            
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1, -1,  1));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(2, -1, -1), 3, 1);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(2,  0, -1), 2, 2);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(2, -1,  0), 1, 3);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(2,  0,  0), 0, 4);

            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(2, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(2,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(2, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(2, -1,  1));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(4, -1, -1), 15, 1);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(4,  0, -1), 10, 2);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(4, -1,  0), 5, 3);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer0.segment(4,  0,  0), 0, 4);

            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(4, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(4,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(4, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(4, -1,  1));
            
            // layer 1: depth 2, tiles 5 thru 20
            //  5  6  9 10
            //  7  8 11 12
            //      *
            // 13 14 17 18
            // 15 16 19 20
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(1, -2,  1), 0, 15);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(1, -1, -1), 0,  8);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(1,  0, -2), 0,  9);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(1,  1,  0), 0, 18);

            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(1, -3,  0));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(1,  2,  0));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(1,  0, -3));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(1,  0,  2));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(2, -1, -1), 0,
                                         5, 6,
                                         7, 8);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(2,  0, -1), 0,
                                          9, 10,
                                         11, 12);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(2, -1,  0), 0,
                                         13, 14,
                                         15, 16);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(2,  0,  0), 0,
                                         17, 18,
                                         19, 20);
            
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(2, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(2,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(2, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(2, -1,  1));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(4, -1, -1), 12,
                                         5, 6,
                                         7, 8);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(4,  0, -1),  8,
                                          9, 10,
                                         11, 12);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(4, -1,  0),  4,
                                         13, 14,
                                         15, 16);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(4,  0,  0),  0,
                                         17, 18,
                                         19, 20);
            
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(4, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(4,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(4, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(4, -1,  1));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(8, -1, -1), 60,
                                         5, 6,
                                         7, 8);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(8,  0, -1), 40,
                                          9, 10,
                                         11, 12);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(8, -1,  0), 20,
                                         13, 14,
                                         15, 16);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer1.segment(8,  0,  0),  0,
                                         17, 18,
                                         19, 20);
            
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(8, -2, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(8,  1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(8, -1, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer1.segment(8, -1,  1));

            // layer 2: depth 3, tiles 21 thru 84
            // 21-24 25-28   37-40 41-44
            // 29-32 33-36   45-48 49-52
            //             *
            // 53-56 57-60   69-72 73-76
            // 61-64 65-68   77-80 81-84
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(2, -1, -1), 0,
                                         33, 34,
                                         35, 36);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(2,  1,  0), 0,
                                         73, 74,
                                         75, 76);
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(2, -5, -5));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(2,  4,  4));

            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(4, -1, -1), 0,
                                         21, 22, 23, 24,
                                         25, 26, 27, 28,
                                         29, 30, 31, 32,
                                         33, 34, 35, 36);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(4,  0,  0), 0,
                                         69, 70, 71, 72,
                                         73, 74, 75, 76,
                                         77, 78, 79, 80,
                                         81, 82, 83, 84);
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(4, -2, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(4,  1,  1));
            
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(8, -1, -1), 48,
                                         21, 22, 23, 24,
                                         25, 26, 27, 28,
                                         29, 30, 31, 32,
                                         33, 34, 35, 36);
            _MEGA_CPPUNIT_ASSERT_SEGMENT(layer2.segment(8,  0,  0), 0,
                                         69, 70, 71, 72,
                                         73, 74, 75, 76,
                                         77, 78, 79, 80,
                                         81, 82, 83, 84);
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(8, -2, -2));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer2.segment(8,  1,  1));
        }
        
        void testLayerGetSegmentEmptyLayer()
        {
            Owner<Canvas> newCanvas = Canvas::create();
            CPPUNIT_ASSERT(newCanvas->layers().size() == 1);
            Layer layer0 = newCanvas->layers()[0];

            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1, -1, -1));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(1,  0,  0));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(2,  0,  0));
            _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY(layer0.segment(4,  0,  0));
        }
        
#undef _MEGA_CPPUNIT_ASSERT_SEGMENT
#undef _MEGA_CPPUNIT_ASSERT_SEGMENT_EMPTY
        
        void testVerifyTiles()
        {
            std::string error;
            Owner<Canvas> canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvas);
            
            bool ok = canvas->verifyTiles(&error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(ok);
        }
        
        void testLoadTile()
        {
            std::string error;
            Owner<Canvas> canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvas);
            CPPUNIT_ASSERT_EQUAL(std::size_t(20), canvas->tileCount());
            
            for (size_t i = 1; i <= 20; ++i) {
                auto tile = canvas->loadTile(1, &error);
                CPPUNIT_ASSERT_EQUAL(std::string(""), error);
                CPPUNIT_ASSERT(tile);
            }
        }
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}
