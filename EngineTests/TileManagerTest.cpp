//
//  TileManagerTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/24/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "EngineTests/GLTest.hpp"
#include "Engine/Canvas.hpp"
#include "Engine/TileManager.hpp"
#include <string>

namespace Mega { namespace test {
    class TileManagerTest : public GLContextTestFixture {
        CPPUNIT_TEST_SUITE(TileManagerTest);
        CPPUNIT_TEST(testRequire);
        CPPUNIT_TEST_SUITE_END();
        
        Owner<Canvas> canvas;
        Owner<TileManager> tileManager;
    public:
        void setUp() override
        {
            this->GLContextTestFixture::setUp();
            
            std::string error;
            canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            if (!canvas)
                throw std::runtime_error(error);
            
            tileManager = TileManager::create(canvas.get());
        }
        
        void tearDown() override
        {
            tileManager.reset();
            canvas.reset();
            this->GLContextTestFixture::tearDown();
        }
        
        void testRequire()
        {
            tileManager->require(Vec{0.0, 0.0}, Vec{128.0, 128.0});
            CPPUNIT_ASSERT(tileManager->isTileReady(1));
            CPPUNIT_ASSERT(tileManager->isTileReady(2));
            CPPUNIT_ASSERT(tileManager->isTileReady(3));
            CPPUNIT_ASSERT(tileManager->isTileReady(4));
            CPPUNIT_ASSERT(tileManager->isTileReady(7));
            CPPUNIT_ASSERT(tileManager->isTileReady(13));
            
            tileManager->require(Vec{444.0, -64.0}, Vec{128.0, 128.0});
            CPPUNIT_ASSERT(tileManager->isTileReady(8));
            CPPUNIT_ASSERT(tileManager->isTileReady(11));
            CPPUNIT_ASSERT(tileManager->isTileReady(14));
            CPPUNIT_ASSERT(tileManager->isTileReady(17));
        }
    };    
    CPPUNIT_TEST_SUITE_REGISTRATION(TileManagerTest);
}}