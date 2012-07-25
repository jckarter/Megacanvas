//
//  GLMeta.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/1/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_GLMeta_hpp
#define Megacanvas_GLMeta_hpp

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include "Engine/Util/GL.h"
#include "Engine/Util/StructMeta.hpp"
#include <array>

namespace Mega {
    enum class GLError : GLenum {
        NO_ERROR = GL_NO_ERROR,
        INVALID_ENUM = GL_INVALID_ENUM,
        INVALID_FRAMEBUFFER_OPERATION = GL_INVALID_FRAMEBUFFER_OPERATION,
        INVALID_INDEX = GL_INVALID_INDEX,
        INVALID_OPERATION = GL_INVALID_OPERATION,
        INVALID_VALUE = GL_INVALID_VALUE,
        OUT_OF_MEMORY = GL_OUT_OF_MEMORY
    };

#ifdef NDEBUG
#define MEGA_ASSERT_GL_NO_ERROR ((void)0)
#else
#define MEGA_ASSERT_GL_NO_ERROR do { GLError glError = GLError(glGetError()); assert(glError == GLError::NO_ERROR); } while(0)
#endif

    template<typename T> struct Pad { T pad; };
    
    struct GLVertexType { GLenum type; GLint size; GLboolean normalized; bool isPadding; };

    template<GLenum Type, GLboolean Normalized>
    struct GLScalarVertexTraits {
        constexpr static GLenum type = Type;
        constexpr static GLint size = 1;
        constexpr static GLboolean normalized = Normalized;
        constexpr static bool isPadding = false;
    };

    template<typename> struct _GLVertexTraits;
    template<>
    struct _GLVertexTraits<float> : GLScalarVertexTraits<GL_FLOAT, GL_FALSE> {};
    template<>
    struct _GLVertexTraits<std::uint8_t> : GLScalarVertexTraits<GL_UNSIGNED_BYTE, GL_TRUE> {};

    template<typename T, GLint N>
    struct _GLVertexTraits<T[N]> : _GLVertexTraits<T> {
        constexpr static GLint size = N;
    };
    
    template<typename T>
    struct _GLVertexTraits<Pad<T>> : _GLVertexTraits<T> {
        constexpr static bool isPadding = true;
    };

    template<typename T>
    struct GLVertexTraits : _GLVertexTraits<T> {
        static GLVertexType value() { return {_GLVertexTraits<T>::type, _GLVertexTraits<T>::size, _GLVertexTraits<T>::normalized, _GLVertexTraits<T>::isPadding}; }
    };
    
    struct GLSLTypeInfo { char const *name; bool isPadding; };

    template<typename T>
    struct GLSLType {
        static GLSLTypeInfo value() { return {"float", false}; }
    };
    template<typename T>
    struct GLSLType<T[2]> {
        static GLSLTypeInfo value() { return {"vec2", false}; }
    };
    template<typename T>
    struct GLSLType<T[3]> {
        static GLSLTypeInfo value() { return {"vec3", false}; }
    };
    template<typename T>
    struct GLSLType<T[4]> {
        static GLSLTypeInfo value() { return {"vec4", false}; }
    };
    template<typename T>
    struct GLSLType<Pad<T>> {
        static GLSLTypeInfo value() { return {"<<PADDING>>", true}; }
    };
    
    template<typename T>
    struct NoTrait {
        static int value() { return 0; }
    };

    template<typename T>
    bool bindVertexAttributes(GLuint prog)
    {
        bool ok = true;
        each_field_metadata<T, GLVertexTraits>([prog, &ok](char const *name,
                                                            std::size_t size,
                                                            std::size_t offset,
                                                            GLVertexType info) {
            if (info.isPadding)
                return;
            
            GLint location = glGetAttribLocation(prog, name);
            if (location == -1) {
                ok = false;
                return;
            }
            glVertexAttribPointer(location, info.size, info.type, info.normalized,
                                  sizeof(T), reinterpret_cast<const GLvoid*>(offset));
            glEnableVertexAttribArray(location);
        });
        return ok;
    }
    
    template<typename T>
    bool getUniformLocations(GLuint prog, T *outFields)
    {
        bool ok = true;
        each_field(*outFields, [prog, &ok](char const *name, GLint &field) {
            field = glGetUniformLocation(prog, name);
            if (field == -1) {
                ok = false;
                return;
            }
        });
        return ok;
    }

    struct GLProgram {
        GLuint program = 0, vertexShader = 0, fragmentShader = 0;
        std::string basename;
        
        explicit GLProgram(llvm::StringRef basename) : basename(basename) {}
        
        GLProgram(const GLProgram&) = delete;
        void operator=(const GLProgram&) = delete;
        
        GLProgram(GLProgram &&x)
        : program(x.program), vertexShader(x.vertexShader), fragmentShader(x.fragmentShader)
        {
            x.program = x.vertexShader = x.fragmentShader = 0;
        }
        
        GLProgram &operator=(GLProgram &&x)
        {
            std::swap(program, x.program);
            std::swap(vertexShader, x.vertexShader);
            std::swap(fragmentShader, x.fragmentShader);
            return *this;
        }
        
        ~GLProgram() { unload(); }
        
        bool compile(std::string *outError);
        bool link(std::string *outError);
        
        void unload();
        
        bool compileAndLink(std::string *outError)
        {
            if (!compile(outError))
                return false;
            if (!link(outError)) {
                unload();
                return false;
            }
            return true;
        }
        
        explicit operator bool() const { return program != 0; }
        operator GLuint() const { return program; }
        
        GLProgram like() const { return GLProgram(basename); }
    };
    
    inline void bindTextureUnitTarget(GLuint unit, GLenum target, GLuint name)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, name);
    }
    
    template<void Gen(GLsizei, GLuint*), void Delete(GLsizei, const GLuint*)>
    struct GLResource {
        GLuint name = 0;
        
        GLResource() {}
        
        GLResource(const GLResource&) = delete;
        void operator=(const GLResource&) = delete;
        
        GLResource(GLResource &&x) : name(x.value) { x.value = 0; }
        GLResource &operator=(GLResource &&x) { std::swap(name, x.value); return *this; }
        
        ~GLResource() {
            if (name) {
                Delete(1, &name);
                MEGA_ASSERT_GL_NO_ERROR;
            }
        }
        
        void gen() {
            Gen(1, &name);
            MEGA_ASSERT_GL_NO_ERROR;
            assert(name);
        }
        
        explicit operator bool() const { return name != 0; }
        operator GLuint() const { return name; }
    };
    
    template<typename> struct FlipFlop;
    
    template<void Gen(GLsizei, GLuint*), void Delete(GLsizei, const GLuint*)>
    struct FlipFlop<GLResource<Gen, Delete>> {
        std::array<GLuint,2> names = {0,0};
        int nextIndex = 0;
        
        FlipFlop() {}
        
        FlipFlop(const FlipFlop&) = delete;
        void operator=(const FlipFlop&) = delete;
        
        FlipFlop(FlipFlop &&x)
        : names(x.names), nextIndex(x.nextIndex)
        { x.names[0] = x.names[1] = 0; }
        
        FlipFlop &operator=(FlipFlop &&x) {
            std::swap(names, x.names);
            nextIndex = x.nextIndex;
            return *this;
        }

        ~FlipFlop() {
            if (names[0]) {
                Delete(2, names.data());
                MEGA_ASSERT_GL_NO_ERROR;
            }
        }
        
        void gen() {
            Gen(2, names.data());
            MEGA_ASSERT_GL_NO_ERROR;
            assert(names[0] && names[1]);
        }
        
        explicit operator bool() const { return names[0] != 0; }
        
        GLuint next() {
            assert(names[0] && names[1]);
            int i = nextIndex;
            nextIndex = !nextIndex;
            return names[i];
        }
    };

#define _MEGA_GL_RESOURCE(name) using GL##name = GLResource<glGen##name##s, glDelete##name##s>;
    _MEGA_GL_RESOURCE(Texture)
    _MEGA_GL_RESOURCE(Buffer)
    _MEGA_GL_RESOURCE(Framebuffer)
    _MEGA_GL_RESOURCE(Renderbuffer)
    _MEGA_GL_RESOURCE(VertexArray)
#undef _MEGA_GL_RESOURCE
}

#endif
