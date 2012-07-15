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
        CPPUNIT_TEST(testVertexShaderTileLayout);
        CPPUNIT_TEST(testVertexShaderLayerOriginAndParallax);
        CPPUNIT_TEST(testMappingTexture);
        CPPUNIT_TEST(testCenterPixelAlignment);
        CPPUNIT_TEST_SUITE_END();

        Owner<Canvas> canvas;
        Owner<View> view;

    public:
        void setUp() override
        {
            this->GLContextTestFixture::setUp();
            std::string error;
            this->canvas = Canvas::load("EngineTests/TestData/Test1.mega", &error);
            if (!this->canvas)
                throw std::runtime_error(error);
            this->view = View::create(this->canvas.get());
            if (!this->view->prepare(&error))
                throw std::runtime_error(error);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        }

        void tearDown() override
        {
            this->view = nullptr;
            this->canvas = nullptr;
            this->GLContextTestFixture::tearDown();
        }

        void testMeshContents()
        {
            //Test1.mega has tile size 128, 2 layers 
            this->view->resize(127.0, 127.0);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            
            Priv<View> &priv = this->view.priv();
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            GLint param;
            glBindBuffer(GL_ARRAY_BUFFER, priv.meshBuffer);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(4*2*2*2*sizeof(ViewVertex)), param);
            ViewVertex vertexData[4*2*2*2];
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, 4*2*2*2*sizeof(ViewVertex),
                               reinterpret_cast<GLvoid*>(vertexData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 0].tileCoord, 0.0f, 0.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 3].tileCoord, 0.0f, 0.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 4].tileCoord, 1.0f, 0.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 7].tileCoord, 1.0f, 0.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 8].tileCoord, 0.0f, 1.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[11].tileCoord, 0.0f, 1.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[12].tileCoord, 1.0f, 1.0f, 0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[15].tileCoord, 1.0f, 1.0f, 0.0f);

            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 0].tileCoord, 0.0f, 0.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 3].tileCoord, 0.0f, 0.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 4].tileCoord, 1.0f, 0.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 7].tileCoord, 1.0f, 0.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 8].tileCoord, 0.0f, 1.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+11].tileCoord, 0.0f, 1.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+12].tileCoord, 1.0f, 1.0f, 1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+15].tileCoord, 1.0f, 1.0f, 1.0f);
            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 0].tileCorner,  0.0f,  0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[ 5].tileCorner,  1.0f,  0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[10].tileCorner,  0.0f,  1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[15].tileCorner,  1.0f,  1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 0].tileCorner,  0.0f,  0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+ 5].tileCorner,  1.0f,  0.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+10].tileCorner,  0.0f,  1.0f);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(vertexData[16+15].tileCorner,  1.0f,  1.0f);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, priv.eltBuffer);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &param);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            CPPUNIT_ASSERT_EQUAL(GLint(6*2*2*2*sizeof(GLuint)), param);
            GLuint elementData[6*2*2*2];
            glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 6*2*2*2*sizeof(GLuint), &elementData);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(elementData,
                                             0,  1,  2,  1,  3,  2,
                                             4,  5,  6,  5,  7,  6,
                                             8,  9, 10,  9, 11, 10,
                                             12, 13, 14, 13, 15, 14,
                                             16, 17, 18, 17, 19, 18,
                                             20, 21, 22, 21, 23, 22,
                                             24, 25, 26, 25, 27, 26,
                                             28, 29, 30, 29, 31, 30);
        }
        
        void testMeshSize()
        {
            View view = this->view.get();
            Priv<View> &priv = *view.that;
            view.resize(127.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 2, 2);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view.resize(126.5, 126.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 2, 2);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view.resize(0.5, 0.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 2, 2);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            
            view.resize(127.5, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 3, 3);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view.resize(253.5, 253.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 3, 3);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view.resize(254.0, 254.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 3, 3);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view.resize(254.5, 254.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*4*4), priv.viewTileTotal);            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 4, 4);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view.resize(381.5, 381.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*5*5), priv.viewTileTotal);            
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 5, 5);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            
            view.resize(127.5, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*2), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 3, 2);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            
            view.resize(127.0, 127.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*3), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 2, 3);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            
            view.resize(381.5, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            view.resize(127.0, 381.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
            
            view.resize(254.0, 254.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*3*3), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 3, 3);
            CPPUNIT_ASSERT_EQUAL(GLuint(4), priv.mappingTextureSegmentSize);
            view.zoom(2.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 2, 2);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            view.zoom(0.5);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*5*5), priv.viewTileTotal);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(priv.viewTileCount, 5, 5);
            CPPUNIT_ASSERT_EQUAL(GLuint(8), priv.mappingTextureSegmentSize);
        }
        
        static constexpr std::size_t bufferSize = (sizeof(float[4]) + sizeof(float[3])) * 6*2*2*2;
        
        void setUpForTransformFeedback()
        {
            View view = this->view.get();
            Priv<View> &priv = *view.that;
            llvm::SmallString<16> log;

            const GLchar *transformFeedbackVaryings[] = {
                "gl_Position",
                "frag_texCoord"
            };
            glTransformFeedbackVaryings(priv.program, 2, transformFeedbackVaryings,
                                        GL_SEPARATE_ATTRIBS);
            if (!linkProgram(priv.program, &log))
                throw std::runtime_error(log.c_str());
            priv.updateShaderParams();
            
            view.resize(127.0, 65.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2*2*2), priv.viewTileTotal);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), priv.mappingTextureSegmentSize);
            
            glEnable(GL_RASTERIZER_DISCARD);
            GLenum buf = GL_NONE;
            glDrawBuffers(1, &buf);
            
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            
            GLuint feedbackBuffer;
            glGenBuffers(1, &feedbackBuffer);
            glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffer);
            glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bufferSize, nullptr, GL_DYNAMIC_COPY);
            glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBuffer,
                              0,
                              sizeof(float[4]) * 6*2*2*2);
            glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 1, feedbackBuffer, 
                              sizeof(float[4]) * 6*2*2*2, 
                              sizeof(float[3]) * 6*2*2*2);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            
            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            glBindTexture(GL_TEXTURE_2D_ARRAY, priv.mappingTexture);
            std::uint16_t testMappings[2*4*4] = {
                 0,  1,  2,  3,
                 4,  5,  6,  7,
                 8,  9, 10, 11,
                12, 13, 14, 15,
                
                16, 17, 18, 19,
                20, 21, 22, 23,
                24, 25, 26, 27,
                28, 29, 30, 31
            };
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            0, 0, 0, 
                            4, 4, 2, 
                            GL_RED, GL_UNSIGNED_SHORT, reinterpret_cast<const GLvoid*>(testMappings));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        }
        
        void runTransformFeedback(std::uint8_t *outBufferData)
        {
            glBeginTransformFeedback(GL_TRIANGLES);
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            this->view->render();
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            glEndTransformFeedback();
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;

            glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bufferSize, 
                               reinterpret_cast<GLvoid*>(outBufferData));
        }
        
        void checkTileCorners(std::uint8_t const *bufferData, std::size_t i,
                              float lowx, float lowy,
                              float tileIndex)
        {
            float const (*positions)[4] = 6*i + reinterpret_cast<float const (*)[4]>(bufferData);
            float const (*texCoords)[3] = 6*i + reinterpret_cast<float const (*)[3]>(bufferData + sizeof(float[4]) * 6*2*2*2);
            CPPUNIT_ASSERT_EQUAL(lowx       , positions[0][0]*63.5f);
            CPPUNIT_ASSERT_EQUAL(lowy       , positions[0][1]*32.5f);
            CPPUNIT_ASSERT_EQUAL(lowx+127.0f, positions[1][0]*63.5f);
            CPPUNIT_ASSERT_EQUAL(lowy       , positions[1][1]*32.5f);
            CPPUNIT_ASSERT_EQUAL(lowx       , positions[2][0]*63.5f);
            CPPUNIT_ASSERT_EQUAL(lowy+127.0f, positions[2][1]*32.5f);
            CPPUNIT_ASSERT_EQUAL(lowx+127.0f, positions[4][0]*63.5f);
            CPPUNIT_ASSERT_EQUAL(lowy+127.0f, positions[4][1]*32.5f);
            
            CPPUNIT_ASSERT_EQUAL(  0.5f/128.0f, texCoords[0][0]);
            CPPUNIT_ASSERT_EQUAL(  0.5f/128.0f, texCoords[0][1]);
            CPPUNIT_ASSERT_EQUAL(127.5f/128.0f, texCoords[1][0]);
            CPPUNIT_ASSERT_EQUAL(  0.5f/128.0f, texCoords[1][1]);
            CPPUNIT_ASSERT_EQUAL(  0.5f/128.0f, texCoords[2][0]);
            CPPUNIT_ASSERT_EQUAL(127.5f/128.0f, texCoords[2][1]);
            CPPUNIT_ASSERT_EQUAL(127.5f/128.0f, texCoords[4][0]);
            CPPUNIT_ASSERT_EQUAL(127.5f/128.0f, texCoords[4][1]);
            
            for (auto j = 0; j < 6; ++j)
                CPPUNIT_ASSERT_EQUAL(tileIndex, texCoords[j][2]);
        }
        
        void testVertexShaderTileLayout()
        {
            View view = this->view.get();
            Priv<View> &priv = *view.that;
            ViewUniforms &uniforms = priv.uniforms;
            
            setUpForTransformFeedback();
            
            std::uint8_t bufferData[bufferSize];
            
            // test canvas has two layers
            // layer 0 (tiles 0 thru 3) has origin 0,0 parallax 1,1
            
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 0,
                             0.0f, 0.0f,
                             0.0f);
            checkTileCorners(bufferData, 1,
                             -127.0f, 0.0f,
                             3.0f);
            checkTileCorners(bufferData, 2,
                             0.0f, -127.0f,
                             12.0f);
            checkTileCorners(bufferData, 3,
                             -127.0, -127.0f,
                             15.0f);

            glUniform2f(uniforms.center, -127.0f, 0.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 0,
                             -127.0f, 0.0f,
                             2.0f);
            checkTileCorners(bufferData, 1,
                             0.0f, 0.0f,
                             3.0f);
            checkTileCorners(bufferData, 2,
                             -127.0f, -127.0f,
                             14.0f);
            checkTileCorners(bufferData, 3,
                             0.0f, -127.0f,
                             15.0f);

            glUniform2f(uniforms.center, -63.5f, 0.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 1,
                             -63.5f, 0.0f,
                             3.0f);
            checkTileCorners(bufferData, 3,
                             -63.5f, -127.0f,
                             15.0f);
            
            glUniform2f(uniforms.center, 63.5f, 0.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 0,
                             -63.5f, 0.0f,
                             0.0f);
            checkTileCorners(bufferData, 2,
                             -63.5f, -127.0f,
                             12.0f);
            
            glUniform2f(uniforms.center, 127.0f, 0.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 0,
                             -127.0f, 0.0f,
                             0.0f);
            checkTileCorners(bufferData, 1,
                             0.0f, 0.0f,
                             1.0f);
            checkTileCorners(bufferData, 2,
                             -127.0f, -127.0f,
                             12.0f);
            checkTileCorners(bufferData, 3,
                             0.0f, -127.0f,
                             13.0f);
        }
        
        void testVertexShaderLayerOriginAndParallax()
        {
            View view = this->view.get();
            Priv<View> &priv = *view.that;
            ViewUniforms const &uniforms = priv.uniforms;
            
            setUpForTransformFeedback();
            
            std::uint8_t bufferData[bufferSize];
            
            // test canvas has two layers
            // layer 1 (tiles 4 thru 7) has origin 508,128 parallax 0.5,0.5

            glUniform2f(uniforms.center, 508.0f, 2.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 4,
                             0.0f, 0.0f,
                             16.0f);
            checkTileCorners(bufferData, 5,
                             -127.0f, 0.0f,
                             19.0f);
            checkTileCorners(bufferData, 6,
                             0.0f, -127.0f,
                             28.0f);
            checkTileCorners(bufferData, 7,
                             -127.0, -127.0f,
                             31.0f);

            glUniform2f(uniforms.center, 508.0f-254.0f, 2.0f);
            runTransformFeedback(bufferData);
            checkTileCorners(bufferData, 4,
                             -127.0f, 0.0f,
                             18.0f);
            checkTileCorners(bufferData, 5,
                             0.0f, 0.0f,
                             19.0f);
            checkTileCorners(bufferData, 6,
                             -127.0f, -127.0f,
                             30.0f);
            checkTileCorners(bufferData, 7,
                             0.0f, -127.0f,
                             31.0f);
        }
        
        void testMappingTexture()
        {
            View view = this->view.get();
            view.resize(127.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(GLuint(2), view.that->mappingTextureSegmentSize);

            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            std::uint16_t mappingTextureData[2][16];
            GLint param;
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &param);
            CPPUNIT_ASSERT_EQUAL(GLint(4), param);
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT, &param);
            CPPUNIT_ASSERT_EQUAL(GLint(4), param);
            glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_DEPTH, &param);
            CPPUNIT_ASSERT_EQUAL(GLint(2), param);

            glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RED, GL_UNSIGNED_SHORT,
                          reinterpret_cast<GLvoid*>(mappingTextureData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[0],
                                             4, 0, 0, 3,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             2, 0, 0, 1);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[1],
                                             0, 0, 13, 14,
                                             0, 0, 15, 16,
                                             0, 0,  5,  6,
                                             0, 0,  7,  8);
            
            view.moveCenter(Vec(508.0, 0.0));
            view.syncTextureStreaming();
            glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RED, GL_UNSIGNED_SHORT, 
                          reinterpret_cast<GLvoid*>(mappingTextureData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[0],
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[1],
                                             17, 18, 13, 14,
                                             19, 20, 15, 16,
                                              9, 10,  5,  6,
                                             11, 12,  7,  8);
            
            view.moveCenter(Vec(-508.0, 0.0));
            view.syncTextureStreaming();
            
            glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RED, GL_UNSIGNED_SHORT,
                          reinterpret_cast<GLvoid*>(mappingTextureData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[0],
                                             4, 0, 0, 3,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             2, 0, 0, 1);
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[1],
                                             0, 0, 13, 14,
                                             0, 0, 15, 16,
                                             0, 0,  5,  6,
                                             0, 0,  7,  8);
            
            view.moveCenter(Vec(-254.0, 0.0));
            view.syncTextureStreaming();
            glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RED, GL_UNSIGNED_SHORT, 
                          reinterpret_cast<GLvoid*>(mappingTextureData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[0],
                                             0, 0, 0, 3,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 1);
            
            view.moveCenter(Vec(254.0, 254.0));
            glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RED, GL_UNSIGNED_SHORT, 
                          reinterpret_cast<GLvoid*>(mappingTextureData));
            MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
            MEGA_CPPUNIT_ASSERT_ARRAY_EQUALS(mappingTextureData[0],
                                             4, 0, 0, 3,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0,
                                             0, 0, 0, 0);
        }
        
        void testCenterPixelAlignment()
        {
            View view = this->view.get();
            Priv<View> &priv = this->view.priv();
            float center[2];

            view.resize(127.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.0), view.center());
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.0), priv.pixelAlign);
            
            view.resize(128.0, 128.0);
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.0), view.center());
            CPPUNIT_ASSERT_EQUAL(Vec(0.5, 0.5), priv.pixelAlign);
            
            view.resize(127.0, 128.0);
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.0), view.center());
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.5), priv.pixelAlign);
            
            view.resize(128.0, 127.0);
            CPPUNIT_ASSERT_EQUAL(Vec(0.0, 0.0), view.center());
            CPPUNIT_ASSERT_EQUAL(Vec(0.5, 0.0), priv.pixelAlign);
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(ViewTest);
}}