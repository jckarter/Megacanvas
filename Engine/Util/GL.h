//
//  GL.h
//  Megacanvas
//
//  Created by Joe Groff on 6/29/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_GL_h
#define Megacanvas_GL_h

#ifdef __APPLE__
#   include <OpenGL/gl3.h>
// prevent Cocoa from loading gl.h
#   define __gl_h_
#else
#   include <GL/glew.h>
#endif

#ifdef __cplusplus
namespace Mega {
    extern "C" {
#endif
        typedef struct Q_gl_context *gl_context_t;
        gl_context_t currentGLContext(void);
        gl_context_t makeSharedGLContext(gl_context_t shareWith, char const **outError);
        void setCurrentGLContext(gl_context_t context);
        void destroyGLContext(gl_context_t context);
#ifdef __cplusplus
    }
}
#endif

#endif
