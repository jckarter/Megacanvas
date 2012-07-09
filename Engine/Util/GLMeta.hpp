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
#include "Engine/Util/GL.h"
#include "Engine/Util/NamedTuple.hpp"

namespace Mega {
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

    namespace {
        // xcode 4.3 can't handle lambdas
        struct _VertexAttributeBinder {
            GLuint program;
            size_t stride;
            bool ok;
            _VertexAttributeBinder(GLuint program, size_t stride) : program(program), stride(stride), ok(true) {}
            void operator()(char const *name, size_t offset, size_t size, GLVertexType info) {
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
            void operator()(char const *name, size_t offset, size_t size, GLSLTypeInfo glslType) {
                if (glslType.isPadding)
                    return;
                codes << "in " << glslType.name << ' ' << name << ";\n";
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
    std::string vertexShaderInputs()
    {
        _VertexShaderInputCollector iter;
        T::template eachField<GLSLType>(iter);
        return std::move(iter.codes.str());
    }

    bool compileProgram(llvm::ArrayRef<llvm::StringRef> vert, llvm::ArrayRef<llvm::StringRef> frag,
                        GLuint *outVert, GLuint *outFrag, GLuint *outProg, llvm::SmallVectorImpl<char> *outLog);
    inline bool compileProgram(llvm::StringRef vert, llvm::StringRef frag,
                               GLuint *outVert, GLuint *outFrag, GLuint *outProg, llvm::SmallVectorImpl<char> *outLog)
    {
        return compileProgram(makeArrayRef(vert), makeArrayRef(frag), outVert, outFrag, outProg, outLog);
    }

    template<typename T>
    inline bool compileProgramAutoInputs(llvm::StringRef vertMain, llvm::StringRef fragMain,
                                         GLuint *outVert, GLuint *outFrag, GLuint *outProg, llvm::SmallVectorImpl<char> *outLog)
    {
        char const *versionString = "#version 150\n";
        std::string inputs = vertexShaderInputs<T>();
        llvm::StringRef vert[] = {versionString, inputs, vertMain};
        llvm::StringRef frag[] = {versionString, fragMain};
        return compileProgram(vert, frag, outVert, outFrag, outProg, outLog);
    }
    
    inline void bindTextureUnitTarget(GLuint unit, GLenum target, GLuint name)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, name);
    }
}

#endif
