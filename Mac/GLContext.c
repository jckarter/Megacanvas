//
//  GLContext.c
//  Megacanvas
//
//  Created by Joe Groff on 7/27/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Util/GL.h"
#include <OpenGL/OpenGL.h>
#include <stdio.h>

gl_context_t currentGLContext(void)
{
    return (gl_context_t)CGLGetCurrentContext();
}

gl_context_t makeSharedGLContext(gl_context_t shareWith, char const **outError)
{
    CGLContextObj shareContext = (CGLContextObj)shareWith;
    CGLPixelFormatObj pixelFormat = CGLGetPixelFormat(shareContext);
    CGLContextObj newContext;
    CGLError code = CGLCreateContext(pixelFormat, shareContext, &newContext);
    
    if (code != kCGLNoError) {
        *outError = CGLErrorString(code);
        return 0;
    } else
        return (gl_context_t)newContext;
}

void setCurrentGLContext(gl_context_t context)
{
    CGLSetCurrentContext((CGLContextObj)context);
}

void destroyGLContext(gl_context_t context)
{
    CGLReleaseContext((CGLContextObj)context);
}