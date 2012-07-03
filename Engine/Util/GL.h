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
#else
#   include <GL/glew.h>
#endif

// create and release offscreen OpenGL contexts for testing
#ifdef __cplusplus
extern "C" {
#endif
int createTestGLContext(void);
void destroyTestGLContext(void);
#ifdef __cplusplus
}
#endif

#endif
