//
//  ViewTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/3/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include "GLTest.hpp"
#include "Engine/Canvas.hpp"
#include "Engine/View-priv.hpp"

namespace Mega { namespace test {
    class ViewTest : public GLContextTestFixture {
        CPPUNIT_TEST_SUITE(ViewTest);
        CPPUNIT_TEST(testMeshContents);
        CPPUNIT_TEST(testMeshSize);
        CPPUNIT_TEST_SUITE_END();

        Owner<Canvas> canvas;

    public:
        void setUp() override
        {
            this->GLContextTestFixture::setUp();
            std::string error;
            this->canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            if (!this->canvas)
                throw std::runtime_error(error);
        }

        void tearDown() override
        {
            canvas = nullptr;
            this->GLContextTestFixture::tearDown();
        }

        void testMeshContents()
        {
            Owner<View> view = View::create(this->canvas.get());
            std::string error;
            if (!view->prepare(&error))
                throw std::runtime_error(error);
            MEGA_GL_ASSERT_NO_ERROR;

            //Test1.mega has tile size 128, 2 layers 
            view->resize(127.0, 127.0);
            MEGA_GL_ASSERT_NO_ERROR;
            
            Priv<View> &priv = view.priv();
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            GLint param;
            glBindBuffer(GL_ARRAY_BUFFER, priv.meshBuffer);
            MEGA_GL_ASSERT_NO_ERROR;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(4*2*2*2*sizeof(ViewVertex)), param);
            ViewVertex vertexData[4*2*2*2];
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, 4*2*2*2*sizeof(ViewVertex),
                               reinterpret_cast<GLvoid*>(vertexData));
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 0].tileCoord, 0.0f, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 3].tileCoord, 0.0f, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 4].tileCoord, 1.0f, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 7].tileCoord, 1.0f, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 8].tileCoord, 0.0f, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[11].tileCoord, 0.0f, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[12].tileCoord, 1.0f, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[15].tileCoord, 1.0f, 1.0f, 0.0f));

            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 0].tileCoord, 0.0f, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 3].tileCoord, 0.0f, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 4].tileCoord, 1.0f, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 7].tileCoord, 1.0f, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 8].tileCoord, 0.0f, 1.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+11].tileCoord, 0.0f, 1.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+12].tileCoord, 1.0f, 1.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+15].tileCoord, 1.0f, 1.0f, 1.0f));
            
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 0].tileCorner,  0.0f,  0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 5].tileCorner,  1.0f,  0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[10].tileCorner,  0.0f,  1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[15].tileCorner,  1.0f,  1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 0].tileCorner,  0.0f,  0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 5].tileCorner,  1.0f,  0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+10].tileCorner,  0.0f,  1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+15].tileCorner,  1.0f,  1.0f));
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, priv.eltBuffer);
            MEGA_GL_ASSERT_NO_ERROR;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(6*2*2*2*sizeof(GLuint)), param);
            GLuint elementData[6*2*2*2];
            glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6*2*2*2*sizeof(GLuint), &elementData);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT(arrayEquals(elementData,
                                        0,  1,  2,  1,  3,  2,
                                        4,  5,  6,  5,  7,  6,
                                        8,  9, 10,  9, 11, 10,
                                       12, 13, 14, 13, 15, 14,
                                       16, 17, 18, 17, 19, 18,
                                       20, 21, 22, 21, 23, 22,
                                       24, 25, 26, 25, 27, 26,
                                       28, 29, 30, 29, 31, 30));
        }
        
        void testMeshSize()
        {
            Owner<View> view = View::create(this->canvas.get());
            Priv<View> &priv = view.priv();
            std::string error;
            if (!view->prepare(&error))
                throw std::runtime_error(error);
            MEGA_GL_ASSERT_NO_ERROR;
            
            view->resize(127.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 2, 2));
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view->resize(126.5, 126.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);            
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 2, 2));
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view->resize(0.5, 0.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 2, 2));
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            
            view->resize(127.5, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 3, 3));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view->resize(253.5, 253.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);            
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 3, 3));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view->resize(254.0, 254.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);            
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 3, 3));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view->resize(254.5, 254.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*4*4), priv.viewTileTotal);            
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 4, 4));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view->resize(381.5, 381.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*5*5), priv.viewTileTotal);            
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 5, 5));
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            
            view->resize(127.5, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*2), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 3, 2));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            
            view->resize(127.0, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*3), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 2, 3));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            
            view->resize(381.5, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            view->resize(127.0, 381.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            
            view->resize(254.0, 254.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 3, 3));
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view->zoom(2.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 2, 2));
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view->zoom(0.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*5*5), priv.viewTileTotal);
            CPPUNIT_ASSERT(arrayEquals(priv.viewTileCount, 5, 5));
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(ViewTest);
}}