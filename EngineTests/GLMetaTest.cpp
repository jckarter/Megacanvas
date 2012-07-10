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
    MEGA_FIELD(position)
    MEGA_FIELD(texcoord)
    MEGA_FIELD(color)
    MEGA_FIELD(scalar)
    MEGA_FIELD(pad)

    class GLMetaContextTest : public GLContextTestFixture {
        CPPUNIT_TEST_SUITE(GLMetaContextTest);
        CPPUNIT_TEST(testGLContext);
        CPPUNIT_TEST(testcompileAndLinkProgram);
        CPPUNIT_TEST(testcompileAndLinkProgramAutoInputs);
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

        void loadTestProgramSource(llvm::StringRef basename, llvm::OwningPtr<llvm::MemoryBuffer> *outVertSource, llvm::OwningPtr<llvm::MemoryBuffer> *outFragSource)
        {
            std::string fullBasename = "EngineTests/TestData/shaders/";
            fullBasename += basename;
            std::string error;
            
            if (!loadProgramSource(fullBasename, outVertSource, outFragSource, &error))
                throw std::runtime_error(error);
        }

        void loadTestProgram(llvm::StringRef basename, GLuint *vert, GLuint *frag, GLuint *prog)
        {
            using namespace llvm;
            OwningPtr<MemoryBuffer> vertSource, fragSource;
            loadTestProgramSource(basename, &vertSource, &fragSource);
            SmallString<16> log;
            if (!compileAndLinkProgram(vertSource->getBuffer(), fragSource->getBuffer(), vert, frag, prog, &log))
                throw std::runtime_error(log.c_str());
            CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError());
        }

        template<typename T>
        void loadTestProgramAutoInputs(llvm::StringRef basename, GLuint *vert, GLuint *frag, GLuint *prog)
        {
            using namespace llvm;
            OwningPtr<MemoryBuffer> vertSource, fragSource;
            loadTestProgramSource(basename, &vertSource, &fragSource);
            SmallString<16> log;
            if (!compileAndLinkProgramAutoInputs<T>(vertSource->getBuffer(), fragSource->getBuffer(), vert, frag, prog, &log))
                throw std::runtime_error(log.c_str());
            CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError());
        }

        void testcompileAndLinkProgram()
        {
            GLuint vert, frag, prog;
            loadTestProgram("test1", &vert, &frag, &prog);

            CPPUNIT_ASSERT(glIsProgram(prog));
            CPPUNIT_ASSERT(glIsShader(vert));
            CPPUNIT_ASSERT(glIsShader(frag));

            GLint status;
            glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetProgramiv(prog, GL_LINK_STATUS, &status);
            CPPUNIT_ASSERT(status);

            GLint type;
            glGetShaderiv(vert, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_VERTEX_SHADER);
            glGetShaderiv(frag, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_FRAGMENT_SHADER);
        }

        void testcompileAndLinkProgramAutoInputs()
        {
            using TestVertex = NamedTuple<position<float[3]>, texcoord<float[2]>, color<std::uint8_t[4]>>;

            GLuint vert, frag, prog;
            loadTestProgramAutoInputs<TestVertex>("test1-auto", &vert, &frag, &prog);

            CPPUNIT_ASSERT(glIsProgram(prog));
            CPPUNIT_ASSERT(glIsShader(vert));
            CPPUNIT_ASSERT(glIsShader(frag));

            GLint status;
            glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
            CPPUNIT_ASSERT(status);
            glGetProgramiv(prog, GL_LINK_STATUS, &status);
            CPPUNIT_ASSERT(status);

            GLint type;
            glGetShaderiv(vert, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_VERTEX_SHADER);
            glGetShaderiv(frag, GL_SHADER_TYPE, &type);
            CPPUNIT_ASSERT(type == GL_FRAGMENT_SHADER);

        }

        void testBindVertexAttributes()
        {
            using TestVertex = NamedTuple<position<float[3]>, texcoord<float[2]>, color<std::uint8_t[4]>, pad<Pad<float>>>;

            GLuint vert, frag, prog;
            loadTestProgram("test1", &vert, &frag, &prog);

            glUseProgram(prog);

            GLuint buffer;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TestVertex), nullptr, GL_STATIC_DRAW);

            GLuint array;
            glGenVertexArrays(1, &array);
            glBindVertexArray(array);

            bool ok = bindVertexAttributes<TestVertex>(prog);
            CPPUNIT_ASSERT(ok);

            CPPUNIT_ASSERT_EQUAL(GLenum(GL_NO_ERROR), glGetError());

            CPPUNIT_ASSERT_EQUAL(GLint(-1), glGetAttribLocation(prog, "pad"));

            GLint param;
            GLvoid *pointerParam;
            
#define _MEGA_ASSERT_ATTRIBUTE(name, type, normalized, size) \
            GLuint name##Index = glGetAttribLocation(prog, #name); \
            CPPUNIT_ASSERT(name##Index != -1); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &param); \
            CPPUNIT_ASSERT(param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &param); \
            CPPUNIT_ASSERT_EQUAL(GLint(sizeof(TestVertex)), param); \
            glGetVertexAttribiv(name##Index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &param); \
            CPPUNIT_ASSERT_EQUAL(GLint(buffer), param); \
            glGetVertexAttribPointerv(name##Index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pointerParam); \
            CPPUNIT_ASSERT_EQUAL(TestVertex::offset_of<name>(), reinterpret_cast<size_t>(pointerParam)); \
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

    class GLMetaNoContextTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(GLMetaNoContextTest);
        CPPUNIT_TEST(testVertexShaderInputs);
        CPPUNIT_TEST_SUITE_END();
    public:
        void setUp() override { }
        void tearDown() override { }
        void testVertexShaderInputs()
        {
            using SomeVertex = NamedTuple<position<float[3]>, texcoord<float[2]>, color<std::uint8_t[4]>, scalar<float>, pad<Pad<float>>>;

            CPPUNIT_ASSERT_EQUAL(std::string("in vec3 position;\n"
                                             "in vec2 texcoord;\n"
                                             "in vec4 color;\n"
                                             "in float scalar;\n"), vertexShaderInputs<SomeVertex>());
        }
    };
    CPPUNIT_TEST_SUITE_REGISTRATION(GLMetaContextTest);
    CPPUNIT_TEST_SUITE_REGISTRATION(GLMetaNoContextTest);

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
    
    void GLContextTestFixture::setUpTestFramebuffer()
    {
        GLuint fb, rb;
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
        glGenRenderbuffers(1, &rb);
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 128, 128);
        MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
        MEGA_CPPUNIT_ASSERT_GL_NO_ERROR;
        CPPUNIT_ASSERT_EQUAL(GLenum(GL_FRAMEBUFFER_COMPLETE),
                             glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
    }
    
}}