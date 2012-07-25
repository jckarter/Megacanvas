//
//  GLMetaTest.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/1/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include "Engine/Util/GLMeta.hpp"
#include "GLTest.hpp"

namespace Mega { namespace test {
    class GLMetaContextTest : public GLContextTestFixture {
        CPPUNIT_TEST_SUITE(GLMetaContextTest);
        CPPUNIT_TEST(testGLContext);
        CPPUNIT_TEST(testProgram);
        CPPUNIT_TEST(testBindVertexAttributes);
        CPPUNIT_TEST_SUITE_END();

    public:
        void testGLContext()
        {
            CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError());

            GLint maj, min;
            glGetIntegerv(GL_MAJOR_VERSION, &maj);
            glGetIntegerv(GL_MINOR_VERSION, &min);
            CPPUNIT_ASSERT(maj > 3 || (maj == 3 && min >= 2));
        }

        void testProgram()
        {
            GLProgram program("EngineTests/TestData/shaders/test1");
            
            CPPUNIT_ASSERT(!program);
            
            std::string error;
            if (!program.compileAndLink(&error)) {
                throw std::runtime_error(error);
            }
            
            CPPUNIT_ASSERT(program);
            
            CPPUNIT_ASSERT(glIsProgram(program.program));
            CPPUNIT_ASSERT(glIsShader(program.vertexShader));
            CPPUNIT_ASSERT(glIsShader(program.fragmentShader));

            GLint status;
            glGetShaderiv(program.vertexShader, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetShaderiv(program.fragmentShader, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetProgramiv(program.program, GL_LINK_STATUS, &status);
            CPPUNIT_ASSERT(status);

            GLint type;
            glGetShaderiv(program.vertexShader, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_VERTEX_SHADER);
            glGetShaderiv(program.fragmentShader, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_FRAGMENT_SHADER);
        }

        void testBindVertexAttributes()
        {
#define MEGA_FIELDS_TestVertex(x) \
    x(position, float[3])\
    x(texcoord, float[2])\
    x(color, std::uint8_t[4])\
    x(pad, Pad<float>)
            MEGA_STRUCT(TestVertex)

            GLProgram program("EngineTests/TestData/shaders/test1");
            std::string error;
            if (!program.compileAndLink(&error)) {
                throw std::runtime_error(error);
            }

            glUseProgram(program);

            GLuint buffer;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TestVertex), nullptr, GL_STATIC_DRAW);

            GLuint array;
            glGenVertexArrays(1, &array);
            glBindVertexArray(array);

            bool ok = bindVertexAttributes<TestVertex>(program);
            CPPUNIT_ASSERT(ok);

            CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError());

            CPPUNIT_ASSERT_EQUAL(GLint(-1), glGetAttribLocation(program, "pad"));

            GLint param;
            GLvoid *pointerParam;
            
#define _MEGA_ASSERT_ATTRIBUTE(name, type, normalized, size) \
            GLuint name##Index = glGetAttribLocation(program, #name); \
            CPPUNIT_ASSERT(name##Index != -1); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &param); \
            CPPUNIT_ASSERT(param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &param); \
            CPPUNIT_ASSERT_EQUAL(GLint(sizeof(TestVertex)), param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &param); \
            CPPUNIT_ASSERT_EQUAL(GLint(buffer), param); \
            glGetVertexAttribPointerv(name##Index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointerParam); \
            CPPUNIT_ASSERT_EQUAL(offsetof(TestVertex, name), reinterpret_cast<std::size_t>(pointerParam)); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &param); \
            CPPUNIT_ASSERT_EQUAL(type, param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &param); \
            CPPUNIT_ASSERT_EQUAL(normalized, param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &param); \
            CPPUNIT_ASSERT_EQUAL(size, param);

            _MEGA_ASSERT_ATTRIBUTE(position, GL_FLOAT, GL_FALSE, 3)
            _MEGA_ASSERT_ATTRIBUTE(texcoord, GL_FLOAT, GL_FALSE, 2)
            _MEGA_ASSERT_ATTRIBUTE(color, GL_UNSIGNED_BYTE, GL_TRUE, 4)
#undef _MEGA_ASSERT_ATTRIBUTE
        }
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(GLMetaContextTest);

    void GLContextTestFixture::setUpTestFramebuffer()
    {
        GLuint fb, rb;
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
        glGenRenderbuffers(1, &rb);
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_SRGB8_ALPHA8, 128, 128);
        MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
        MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        CPPUNIT_ASSERT_EQUAL(GLenum(GL_FRAMEBUFFER_COMPLETE),
                             glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
    }
    
}}

namespace Mega {
    std::ostream &operator<<(std::ostream &os, GLError err)
    {
        switch (GLenum(err)) {
            case GL_NO_ERROR:
                os << "GL_NO_ERROR";
                break;
            case GL_INVALID_ENUM:
                os << "GL_INVALID_ENUM";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                os << "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_INVALID_INDEX:
                os << "GL_INVALID_INDEX";
                break;
            case GL_INVALID_OPERATION:
                os << "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_VALUE:
                os << "GL_INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                os << "GL_OUT_OF_MEMORY";
                break;
            default:
                os << "0x" << std::hex << GLenum(err) << std::dec;
                break;
        }
        return os;
    }
    
    std::ostream &operator<<(std::ostream &os, Vec vec)
    {
        return os << "Vec{" << vec.x << ", " << vec.y << "}";
    }
}