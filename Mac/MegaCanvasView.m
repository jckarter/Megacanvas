//
//  MegaCanvasView.m
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import "MegaCanvasView.h"
#include <OpenGL/gl.h>

@implementation MegaCanvasView

- (void)awakeFromNib
{
    NSOpenGLPixelFormatAttribute pfa[] = {
        NSOpenGLPFADepthSize, 32,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };
    
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:pfa];
    if (!pf) {
        NSLog(@"failed to create pixel buffer");
        [self setPixelFormat:nil];
        return;
    }
    [self setPixelFormat:pf];
}

- (void)reshape
{
    [super reshape];
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];
    NSLog(@"prepareOpenGL");
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
}

- (void)drawRect:(NSRect)dirtyRect
{
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        NSLog(@"gl error 0x%x", error);

    [[self openGLContext] flushBuffer];
}

@end
