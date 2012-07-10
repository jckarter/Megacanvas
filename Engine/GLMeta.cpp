//
//  GLMeta.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/2/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Util/GLMeta.hpp"
#include <llvm/Support/raw_ostream.h>

namespace Mega {
    namespace {
        template<void glGet_iv(GLuint, GLenum, GLint*), void glGet_InfoLog(GLuint, GLsizei, GLsizei*, GLchar*)>
        void getLog(GLuint object, llvm::SmallVectorImpl<char> *outLog)
        {
            GLint length;
            glGet_iv(object, GL_INFO_LOG_LENGTH, &length);
            outLog->resize(length);
            glGet_InfoLog(object, length, NULL, outLog->data());
        }

        bool compileShader(llvm::ArrayRef<llvm::StringRef> sources, GLenum type, GLuint *outShader, llvm::SmallVectorImpl<char> *outLog)
        {
            using namespace llvm;
            *outShader = glCreateShader(type);
            SmallVector<GLchar const*, 2> sourceCodes;
            SmallVector<GLint, 2> lengths;
            for (auto source : sources) {
                sourceCodes.push_back(reinterpret_cast<GLchar const*>(source.data()));
                lengths.push_back(source.size());
            }
            glShaderSource(*outShader, sources.size(), sourceCodes.data(), lengths.data());
            glCompileShader(*outShader);
            GLint status;
            glGetShaderiv(*outShader, GL_COMPILE_STATUS, &status);
            if (!status) {
                getLog<glGetShaderiv, glGetShaderInfoLog>(*outShader, outLog);
                glDeleteShader(*outShader);
                *outShader = 0;
                return false;
            }
            return true;
        }
    }
    
    bool loadProgramSource(llvm::StringRef baseName, 
                           llvm::OwningPtr<llvm::MemoryBuffer> *outVertSource, 
                           llvm::OwningPtr<llvm::MemoryBuffer> *outFragSource, 
                           std::string *outError)
    {
        using namespace llvm;
        using namespace std;
        string vertName = baseName, fragName = baseName;
        vertName += ".v.glsl"; fragName += ".f.glsl";
        
        error_code error;
        error = MemoryBuffer::getFile(vertName, *outVertSource);
        if (error) {
            *outError = error.message();
            return false;
        }
        error = MemoryBuffer::getFile(fragName, *outFragSource);
        if (error) {
            *outError = error.message();
            outVertSource->reset();
            return false;
        }
        return true;
    }
    
    bool compileProgram(llvm::ArrayRef<llvm::StringRef> vert, llvm::ArrayRef<llvm::StringRef> frag,
                        GLuint *outVert, GLuint *outFrag, GLuint *outProg,
                        llvm::SmallVectorImpl<char> *outLog)
    {
        *outFrag = *outVert = *outProg = 0;
        outLog->clear();
        if (!compileShader(vert, GL_VERTEX_SHADER, outVert, outLog))
            goto error;
        if (!compileShader(frag, GL_FRAGMENT_SHADER, outFrag, outLog))
            goto error_after_compiling_vert;
        *outProg = glCreateProgram();
        glAttachShader(*outProg, *outVert);
        glAttachShader(*outProg, *outFrag);

        return true;

    error_after_compiling_frag:
        glDeleteShader(*outFrag);
        outFrag = 0;
    error_after_compiling_vert:
        glDeleteShader(*outVert);
        outVert = 0;
    error:
        return false;
    }
    
    bool linkProgram(GLuint prog, llvm::SmallVectorImpl<char> *outLog)
    {
        glLinkProgram(prog);
        GLint status;
        glGetProgramiv(prog, GL_LINK_STATUS, &status);
        if (!status) {
            getLog<glGetProgramiv, glGetProgramInfoLog>(prog, outLog);
            return false;
        }
        return true;
    }
    
    void destroyProgram(GLuint vert, GLuint frag, GLuint prog)
    {
        glDetachShader(prog, vert);
        glDetachShader(prog, frag);
        glDeleteProgram(prog);
        glDeleteShader(vert);
        glDeleteShader(frag);
    }
}