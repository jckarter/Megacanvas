//
//  TestGLContext.c
//  Megacanvas
//
//  Created by Joe Groff on 7/1/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Util/GL.h"
#include <OpenGL/OpenGL.h>
#include <stdio.h>

static CGLContextObj g_context;

int createTestGLContext(void) {
    CGLPixelFormatAttribute attributes[] = {
        kCGLPFAOpenGLProfile, kCGLOGLPVersion_3_2_Core,
        kCGLPFADoubleBuffer,
        0
    };
    CGLPixelFormatObj pixelFormat;
    GLint numScreens;
    CGLError error;

    error = CGLChoosePixelFormat(attributes, &pixelFormat, &numScreens);
    if (error != kCGLNoError)
        return 0;

    error = CGLCreateContext(pixelFormat, 0, &g_context);
    CGLReleasePixelFormat(pixelFormat);
    if (error != kCGLNoError)
        return 0;

    CGLSetCurrentContext(g_context);
    return 1;
}

void destroyTestGLContext(void) {
    CGLReleaseContext(g_context);
}