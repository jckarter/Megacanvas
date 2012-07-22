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
#include "Engine/Util/NamedTuple.hpp"

namespace Mega {
    template<template<typename> class...Field>
    using UniformTuple = NamedTuple<Field<GLint>...>;

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

    namespace {
        // xcode 4.3 can't handle lambdas
        struct _VertexAttributeBinder {
            GLuint program;
            std::size_t stride;
            bool ok;
            _VertexAttributeBinder(GLuint program, std::size_t stride) : program(program), stride(stride), ok(true) {}
            void operator()(char const *name, std::size_t offset, std::size_t size, GLVertexType info) {
                if (info.isPadding)
                    return;
                
                GLint location = glGetAttribLocation(program, name);
                if (location == -1) {
                    ok = false;
                    return;
                }
                glVertexAttribPointer(location, info.size, info.type, info.normalized,
                                      stride, reinterpret_cast<const GLvoid*>(offset));
                glEnableVertexAttribArray(location);
            }
        };

        struct _VertexShaderInputCollector {
            std::string code;
            llvm::raw_string_ostream codes;
            _VertexShaderInputCollector() : codes(code) {}
            void operator()(char const *name, std::size_t offset, std::size_t size, GLSLTypeInfo glslType) {
                if (glslType.isPadding)
                    return;
                codes << "in " << glslType.name << ' ' << name << ";\n";
            }
        };
        
        template<typename T>
        struct _UniformLocationGetter {
            GLuint program;
            T &uniforms;
            bool ok;
            _UniformLocationGetter(GLuint program, T &uniforms) : program(program), uniforms(uniforms), ok(true) {}
            
            void operator()(char const *name, GLint &field) {
                field = glGetUniformLocation(program, name);
                if (field == -1) {
                    ok = false;
                    return;
                }
            }
        };
    }

    template<typename T>
    bool bindVertexAttributes(GLuint prog)
    {
        _VertexAttributeBinder iter(prog, sizeof(T));
        T::template eachField<GLVertexTraits>(iter);
        return iter.ok;
    }
    
    template<typename T>
    bool getUniformLocations(GLuint prog, T *outFields)
    {
        _UniformLocationGetter<T> iter(prog, *outFields);
        outFields->template eachInstanceField(iter);
        return iter.ok;
    }

    template<typename T>
    std::string vertexShaderInputs()
    {
        _VertexShaderInputCollector iter;
        T::template eachField<GLSLType>(iter);
        return std::move(iter.codes.str());
    }
    
    bool loadProgramSource(llvm::StringRef baseName, llvm::OwningPtr<llvm::MemoryBuffer> *outVertSource, llvm::OwningPtr<llvm::MemoryBuffer> *outFragSource, std::string *outError);
    
    bool compileProgram(llvm::ArrayRef<llvm::StringRef> vert, llvm::ArrayRef<llvm::StringRef> frag,
                        GLuint *outVert, GLuint *outFrag, GLuint *outProg,
                        llvm::SmallVectorImpl<char> *outLog);
    bool linkProgram(GLuint prog, llvm::SmallVectorImpl<char> *outLog);
    void destroyProgram(GLuint vert, GLuint frag, GLuint prog);
    
    inline bool compileAndLinkProgram(llvm::ArrayRef<llvm::StringRef> vert,
                                      llvm::ArrayRef<llvm::StringRef> frag,
                                      GLuint *outVert, GLuint *outFrag, GLuint *outProg,
                                      llvm::SmallVectorImpl<char> *outLog)
    {
        if (!compileProgram(vert, frag, outVert, outFrag, outProg, outLog))
            return false;
        if (!linkProgram(*outProg, outLog)) {
            destroyProgram(*outVert, *outFrag, *outProg);
            return false;
        }
        return true;
    }
    inline bool compileAndLinkProgram(llvm::StringRef vert, llvm::StringRef frag,
                                      GLuint *outVert, GLuint *outFrag, GLuint *outProg,
                                      llvm::SmallVectorImpl<char> *outLog)
    {
        return compileAndLinkProgram(makeArrayRef(vert), makeArrayRef(frag), outVert, outFrag, outProg, outLog);
    }

    template<typename T>
    inline bool compileAndLinkProgramAutoInputs(llvm::StringRef vertMain, llvm::StringRef fragMain,
                                                GLuint *outVert, GLuint *outFrag, GLuint *outProg,
                                                llvm::SmallVectorImpl<char> *outLog)
    {
        char const *versionString = "#version 150\n";
        std::string inputs = vertexShaderInputs<T>();
        llvm::StringRef vert[] = {versionString, inputs, vertMain};
        llvm::StringRef frag[] = {versionString, fragMain};
        return compileAndLinkProgram(vert, frag, outVert, outFrag, outProg, outLog);
    }
    
    inline void bindTextureUnitTarget(GLuint unit, GLenum target, GLuint name)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, name);
    }
    
    enum class GLError : GLenum {
        NO_ERROR = GL_NO_ERROR,
        INVALID_ENUM = GL_INVALID_ENUM,
        INVALID_FRAMEBUFFER_OPERATION = GL_INVALID_FRAMEBUFFER_OPERATION,
        INVALID_INDEX = GL_INVALID_INDEX,
        INVALID_OPERATION = GL_INVALID_OPERATION,
        INVALID_VALUE = GL_INVALID_VALUE,
        OUT_OF_MEMORY = GL_OUT_OF_MEMORY
    };
    
    template<typename T>
    struct FlipFlop {
        T members[2];
        int which = 0;
        
        T next() {
            int i = which;
            which = !which;
            return members[i];
        }
    };
}

#ifdef NDEBUG
#define MEGA_ASSERT_GL_NO_ERROR ((void)0)
#else
#define MEGA_ASSERT_GL_NO_ERROR do { GLError glError = GLError(glGetError()); assert(glError == GLError(GL_NO_ERROR)); } while(0)
#endif

#endif
