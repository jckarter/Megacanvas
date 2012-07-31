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

//fixme mac-specific
#include <CoreFoundation/CoreFoundation.h>

namespace Mega { namespace test {
    class CanvasTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(CanvasTest);
        CPPUNIT_TEST(testNewCanvas);
        CPPUNIT_TEST(testLoadCanvasTest1);
        CPPUNIT_TEST(testLoadCanvasFailsWhenNonexistent);
        CPPUNIT_TEST(testLayerGetSegment);
        CPPUNIT_TEST(testLayerGetSegmentEmptyLayer);
        CPPUNIT_TEST(testLayerGetTile);
        CPPUNIT_TEST(testVerifyTiles);
        CPPUNIT_TEST(testLoadTile);
        CPPUNIT_TEST(testLoadTileIntoAsync);
        CPPUNIT_TEST(testBlitIntoEmptySmall);
        CPPUNIT_TEST(testBlitIntoEmptyLarge);
        CPPUNIT_TEST(testBlitBlending);
        CPPUNIT_TEST(testBlitGrowsLayer);
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
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::create(&error);
            Canvas canvas = canvasOwner.get();
            CPPUNIT_ASSERT(canvas.tileLogSize() == 7);
            CPPUNIT_ASSERT(canvas.tileSize() == 128);
            CPPUNIT_ASSERT(canvas.tileArea() == 16384);
            CPPUNIT_ASSERT(canvas.tileCount() == 0);
            CPPUNIT_ASSERT(canvas.layers().size() == 1);
            Layer layer = canvas.layers()[0];
            CPPUNIT_ASSERT(layer.parallax() == (Vec{1.0, 1.0}));
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

            CPPUNIT_ASSERT(layers[0].parallax() == (Vec{1., 1.}));

            CPPUNIT_ASSERT(layers[1].parallax() == (Vec{0.5, 0.5}));

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
        
        void testLayerGetTile()
        {
            using namespace std;
            std::string error;
            Owner<Canvas> canvasOwner = Canvas::load("EngineTests/TestData/test2.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvasOwner);
            Canvas canvas = canvasOwner.get();
            
            // layer 0: depth 1, tiles 1 thru 4
            Layer layer0 = canvas.layers()[0];
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(1), layer0.tile(-1, -1));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(2), layer0.tile( 0, -1));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(3), layer0.tile(-1,  0));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(4), layer0.tile( 0,  0));
            
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), layer0.tile(-2, -1));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), layer0.tile( 1, -1));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), layer0.tile(-1, -2));
            CPPUNIT_ASSERT_EQUAL(Layer::tile_t(0), layer0.tile(-1,  1));
        }
        
        void testLayerGetSegmentEmptyLayer()
        {
            std::string error;
            Owner<Canvas> newCanvas = Canvas::create(&error);
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
            
            std::vector<uint8_t> tile;
            tile.resize(canvas->tileByteSize());
            for (size_t i = 1; i <= 20; ++i) {
                bool ok = canvas->loadTileInto(i, tile, &error);
                CPPUNIT_ASSERT_EQUAL(std::string(""), error);
                CPPUNIT_ASSERT(ok);
                CPPUNIT_ASSERT_EQUAL(canvas->tileByteSize(), tile.size());
            }
        }
        
        void testLoadTileIntoAsync()
        {
            std::string error;
            Owner<Canvas> canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            CPPUNIT_ASSERT_EQUAL(std::string(""), error);
            CPPUNIT_ASSERT(canvas);
            CPPUNIT_ASSERT_EQUAL(std::size_t(20), canvas->tileCount());
            CPPUNIT_ASSERT_EQUAL(std::size_t(128*128*4), canvas->tileByteSize());

            std::vector<llvm::Optional<std::string>> loadedTiles;
            std::vector<std::array<std::uint8_t, 128*128*4>> tileBuffers;
            loadedTiles.resize(20);
            tileBuffers.resize(20);
            
            for (size_t i = 1; i <= 20; ++i) {
                canvas->loadTileIntoAsync(i, {tileBuffers[i-1].begin(), tileBuffers[i-1].end()},
                                          [&loadedTiles, i](bool ok, std::string const &error) {
                                              if (ok)
                                                  loadedTiles[i-1] = "";
                                              else
                                                  loadedTiles[i-1] = error;
                                          });
            }
            
            //fixme mac-specific
            do {
                CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, false);
            } while (std::any_of(loadedTiles.begin(), loadedTiles.end(),
                                 [](llvm::Optional<std::string> const &x) { return !x.hasValue(); }));
            
            for (auto loadedTile : loadedTiles) {
                CPPUNIT_ASSERT(loadedTile.hasValue());
                CPPUNIT_ASSERT_EQUAL(std::string(""), *loadedTile);
            }
        }

#define _MEGA_ASSERT_TILE_VARS \
    Layer::tile_t tile; \
    vector<uint8_t> tileData; \
    tileData.resize(canvas->tileByteSize());\
    array<uint8_t,4> const *tilePixels; \
    bool ok;
        
#define _MEGA_ASSERT_TILE_CONTENTS(tilex, tiley, xpred, ypred) \
    tile = layer0.tile(tilex, tiley); \
    ok = canvas->loadTileInto(tile, tileData, &error); \
    tilePixels = reinterpret_cast<array<uint8_t,4> const *>(tileData.data()); \
    //asm volatile ("int $3\n"); \
    CPPUNIT_ASSERT_EQUAL(string(""), error); \
    CPPUNIT_ASSERT(tileData); \
    for (size_t y = 0; y < 128; ++y) \
        for (size_t x = 0; x < 128; ++x) \
            if (xpred && ypred) \
                CPPUNIT_ASSERT_EQUAL((array<uint8_t,4>{{1,2,3,4}}), tilePixels[y*128+x]); \
            else \
                CPPUNIT_ASSERT_EQUAL((array<uint8_t,4>{{0,0,0,0}}), tilePixels[y*128+x]);

        
        void testBlitIntoEmptySmall()
        {
            using namespace std;
            string error;
            Owner<Canvas> canvas = Canvas::create(&error);
            CPPUNIT_ASSERT_EQUAL(string(""), error);
            CPPUNIT_ASSERT(canvas);
            CPPUNIT_ASSERT_EQUAL(size_t(0), canvas->tileCount());
            CPPUNIT_ASSERT_EQUAL(size_t(128*128*4), canvas->tileByteSize());
            CPPUNIT_ASSERT_EQUAL(size_t(1), canvas->layers().size());
            Layer layer0 = canvas->layers()[0];
            
            unique_ptr<array<uint8_t,4>[]> stuffToBlit(new array<uint8_t,4>[200*200]);
            for (size_t i = 0; i < 200*200; ++i) {
                stuffToBlit[i] = {1,2,3,4};
            }
            
            canvas->blit("test",
                         stuffToBlit.get(),
                         200, 200, 200,
                         0, -15, 130,
                         [](Canvas::pixel_t s, Canvas::pixel_t d) { return s; });

            CPPUNIT_ASSERT_EQUAL((Vec{85, 230}), layer0.origin());
            
            _MEGA_ASSERT_TILE_VARS
            _MEGA_ASSERT_TILE_CONTENTS(-1, -1, x >= 28, y >= 28)
            _MEGA_ASSERT_TILE_CONTENTS( 0, -1, x < 100, y >= 28)
            _MEGA_ASSERT_TILE_CONTENTS(-1,  0, x >= 28, y < 100)
            _MEGA_ASSERT_TILE_CONTENTS( 0,  0, x < 100, y < 100)
        }
        
        void testBlitIntoEmptyLarge()
        {
            using namespace std;
            string error;
            Owner<Canvas> canvas = Canvas::create(&error);
            CPPUNIT_ASSERT_EQUAL(string(""), error);
            CPPUNIT_ASSERT(canvas);
            CPPUNIT_ASSERT_EQUAL(size_t(0), canvas->tileCount());
            CPPUNIT_ASSERT_EQUAL(size_t(128*128*4), canvas->tileByteSize());
            CPPUNIT_ASSERT_EQUAL(size_t(1), canvas->layers().size());
            Layer layer0 = canvas->layers()[0];
            
            unique_ptr<array<uint8_t,4>[]> stuffToBlit(new array<uint8_t,4>[301*301]);
            for (size_t i = 0; i < 301*301; ++i) {
                stuffToBlit[i] = {1,2,3,4};
            }
            
            canvas->blit("test",
                         stuffToBlit.get(),
                         301, 301, 301,
                         0, 1777, -26,
                         [](Canvas::pixel_t s, Canvas::pixel_t d) { return s; });
            
            CPPUNIT_ASSERT_EQUAL((Vec{1777+150, -26+150}), layer0.origin());
            
            _MEGA_ASSERT_TILE_VARS
            _MEGA_ASSERT_TILE_CONTENTS(-2, -2, x >= 106, y >= 106);
            _MEGA_ASSERT_TILE_CONTENTS(-1, -2, true,     y >= 106);
            _MEGA_ASSERT_TILE_CONTENTS( 0, -2, true,     y >= 106);
            _MEGA_ASSERT_TILE_CONTENTS( 1, -2, x <   22, y >= 106);
            _MEGA_ASSERT_TILE_CONTENTS(-2, -1, x >= 106, true);
            _MEGA_ASSERT_TILE_CONTENTS(-1, -1, true,     true);
            _MEGA_ASSERT_TILE_CONTENTS( 0, -1, true,     true);
            _MEGA_ASSERT_TILE_CONTENTS( 1, -1, x <   22, true);
            _MEGA_ASSERT_TILE_CONTENTS(-2,  0, x >= 106, true);
            _MEGA_ASSERT_TILE_CONTENTS(-1,  0, true,     true);
            _MEGA_ASSERT_TILE_CONTENTS( 0,  0, true,     true);
            _MEGA_ASSERT_TILE_CONTENTS( 1,  0, x <   22, true);
            _MEGA_ASSERT_TILE_CONTENTS(-2,  1, x >= 106, y < 22);
            _MEGA_ASSERT_TILE_CONTENTS(-1,  1, true,     y < 22);
            _MEGA_ASSERT_TILE_CONTENTS( 0,  1, true,     y < 22);
            _MEGA_ASSERT_TILE_CONTENTS( 1,  1, x <   22, y < 22);
        }
#undef _MEGA_ASSERT_TILE_VARS
#undef _MEGA_ASSERT_TILE_CONTENTS
        
        void testBlitBlending()
        {
            using namespace std;
            using namespace llvm;
            string error;
            Owner<Canvas> canvas = Canvas::create(&error);
            CPPUNIT_ASSERT_EQUAL(string(""), error);
            CPPUNIT_ASSERT(canvas);
            CPPUNIT_ASSERT_EQUAL(size_t(0), canvas->tileCount());
            CPPUNIT_ASSERT_EQUAL(size_t(128*128*4), canvas->tileByteSize());
            CPPUNIT_ASSERT_EQUAL(size_t(1), canvas->layers().size());
            Layer layer0 = canvas->layers()[0];
            
            unique_ptr<array<uint8_t,4>[]> stuffToBlit1(new array<uint8_t,4>[256*256]);
            fill(&stuffToBlit1[0], &stuffToBlit1[256*256], array<uint8_t,4>{{1,2,3,4}});
            unique_ptr<array<uint8_t,4>[]> stuffToBlit2(new array<uint8_t,4>[128*128]);
            fill(&stuffToBlit2[0], &stuffToBlit2[128*128], array<uint8_t,4>{{5,6,7,8}});
            unique_ptr<array<uint8_t,4>[]> stuffToBlit3(new array<uint8_t,4>[64*64]);
            fill(&stuffToBlit3[0], &stuffToBlit3[64*64], array<uint8_t,4>{{9,10,11,12}});
            
            vector<uint8_t> tileData;
            tileData.resize(canvas->tileByteSize());
            Array2DRef<Canvas::pixel_t> pixels{
                reinterpret_cast<Canvas::pixel_t const*>(tileData.data()),
                canvas->tileSize(), canvas->tileSize()
            };
            
            canvas->blit("test1", stuffToBlit1.get(),
                         256, 256, 256,
                         0, 0, 0,
                         [](Canvas::pixel_t s, Canvas::pixel_t d){ return s; });
            
            canvas->blit("test2", stuffToBlit2.get(),
                         128, 128, 128,
                         0, 1, 1,
                         [](Canvas::pixel_t s, Canvas::pixel_t d) {
                             return Canvas::pixel_t{{
                                 uint8_t(s[0]+d[0]),
                                 uint8_t(s[1]+d[1]),
                                 uint8_t(s[2]+d[2]),
                                 uint8_t(s[3]+d[3])
                             }};
                         });
            
            bool ok = canvas->loadTileInto(layer0.tile(-1, -1), tileData, &error);
            CPPUNIT_ASSERT(ok);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{1,2,3,4}}), pixels[0][0]);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{6,8,10,12}}), pixels[1][1]);
            ok = canvas->loadTileInto(layer0.tile(0, 0), tileData, &error);
            CPPUNIT_ASSERT(ok);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{6,8,10,12}}), pixels[0][0]);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{1,2,3,4}}), pixels[1][1]);
            
            canvas->blit("test3", stuffToBlit3.get(),
                         64, 64, 64,
                         0, 2, 2,
                         [](Canvas::pixel_t s, Canvas::pixel_t d) {
                             return Canvas::pixel_t{{
                                 uint8_t(d[0]-s[0]),
                                 uint8_t(d[1]-s[1]),
                                 uint8_t(d[2]-s[2]),
                                 uint8_t(d[3]-s[3])
                             }};
                         });
            ok = canvas->loadTileInto(layer0.tile(-1, -1), tileData, &error);
            CPPUNIT_ASSERT(ok);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{1,2,3,4}}), pixels[0][0]);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{6,8,10,12}}), pixels[1][1]);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{253,254,255,0}}), pixels[2][2]);
            CPPUNIT_ASSERT_EQUAL((Canvas::pixel_t{{6,8,10,12}}), pixels[66][66]);
        }
        
        void testBlitGrowsLayer()
        {
            CPPUNIT_FAIL("rite me");
        }
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(CanvasTest);
}}
