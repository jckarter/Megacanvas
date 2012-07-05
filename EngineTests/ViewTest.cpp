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
            this->canvas = Canvas::load("Test1.mega", &error);
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
            view->prepare();
            MEGA_GL_ASSERT_NO_ERROR;

            //Test1.mega has tile size 128, 2 layers 
            view->resize(127.0, 127.0);
            MEGA_GL_ASSERT_NO_ERROR;
            
            Priv<View> &priv = view.priv();
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileCount);
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
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 0].tileCoord, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 3].tileCoord, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 4].tileCoord, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 7].tileCoord, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 8].tileCoord, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[11].tileCoord, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[12].tileCoord, 1.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[15].tileCoord, 1.0f, 1.0f));

            CPPUNIT_ASSERT_EQUAL(0.0f, vertexData[ 0].layerIndex);
            CPPUNIT_ASSERT_EQUAL(0.0f, vertexData[15].layerIndex);

            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 0].tileCoord, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 3].tileCoord, 0.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 4].tileCoord, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 7].tileCoord, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+ 8].tileCoord, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+11].tileCoord, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+12].tileCoord, 1.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16+15].tileCoord, 1.0f, 1.0f));
            
            CPPUNIT_ASSERT_EQUAL(1.0f, vertexData[16+ 0].layerIndex);
            CPPUNIT_ASSERT_EQUAL(1.0f, vertexData[16+15].layerIndex);

            CPPUNIT_ASSERT_EQUAL(0.0f, vertexData[ 0].tileCorner);
            CPPUNIT_ASSERT_EQUAL(1.0f, vertexData[ 5].tileCorner);
            CPPUNIT_ASSERT_EQUAL(2.0f, vertexData[10].tileCorner);
            CPPUNIT_ASSERT_EQUAL(3.0f, vertexData[15].tileCorner);
            CPPUNIT_ASSERT_EQUAL(0.0f, vertexData[16+ 0].tileCorner);
            CPPUNIT_ASSERT_EQUAL(1.0f, vertexData[16+ 5].tileCorner);
            CPPUNIT_ASSERT_EQUAL(2.0f, vertexData[16+10].tileCorner);
            CPPUNIT_ASSERT_EQUAL(3.0f, vertexData[16+15].tileCorner);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, priv.eltBuffer);
            MEGA_GL_ASSERT_NO_ERROR;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(6*2*2*2*sizeof(GLuint)), param);
            GLuint elementData[6*2*2*2];
            glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6*2*2*2*sizeof(GLuint), &elementData);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT(arrayEquals(elementData,
                                        0,  2,  1,  1,  2,  3,
                                        4,  6,  5,  5,  6,  7,
                                        8, 10,  9,  9, 10, 11,
                                       12, 14, 13, 13, 14, 15,
                                       16, 18, 17, 17, 18, 19,
                                       20, 22, 21, 21, 22, 23,
                                       24, 26, 25, 25, 26, 27,
                                       28, 30, 29, 29, 30, 31));
        }
        
        void testMeshSize()
        {
            Owner<View> view = View::create(this->canvas.get());
            Priv<View> &priv = view.priv();
            view->prepare();
            MEGA_GL_ASSERT_NO_ERROR;
            
            view->resize(127.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileCount);
            view->resize(126.5, 126.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileCount);            
            view->resize(0.5, 0.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileCount);
            
            view->resize(127.5, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileCount);
            view->resize(253.5, 253.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileCount);            
            view->resize(254.0, 254.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileCount);            
            view->resize(254.5, 254.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*4*4), priv.viewTileCount);            

            GLint param;
            ViewVertex vertexData[4*2*3*2];

            view->resize(127.5, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*2), priv.viewTileCount);
            glBindBuffer(GL_ARRAY_BUFFER, priv.meshBuffer);
            MEGA_GL_ASSERT_NO_ERROR;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(4*2*3*2*sizeof(ViewVertex)), param);
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, 4*2*3*2*sizeof(ViewVertex),
                               reinterpret_cast<GLvoid*>(vertexData));
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 4].tileCoord, 0.0f, 1.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 8].tileCoord, 1.0f, 0.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[16].tileCoord, 2.0f, 0.0f));
            
            view->resize(127.0, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*3), priv.viewTileCount);
            glBindBuffer(GL_ARRAY_BUFFER, priv.meshBuffer);
            MEGA_GL_ASSERT_NO_ERROR;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(4*2*3*2*sizeof(ViewVertex)), param);
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, 4*2*3*2*sizeof(ViewVertex),
                               reinterpret_cast<GLvoid*>(vertexData));
            MEGA_GL_ASSERT_NO_ERROR;
            CPPUNIT_ASSERT(arrayEquals(vertexData[ 8].tileCoord, 0.0f, 2.0f));
            CPPUNIT_ASSERT(arrayEquals(vertexData[12].tileCoord, 1.0f, 0.0f));
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(ViewTest);
}}