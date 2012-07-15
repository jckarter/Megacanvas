//
//  MegaCanvasView.m
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import "MegaCanvasView.hh"
#import "MegaDocument.hh"
#include <cstring>

namespace Mega {
    char const *shaderPath = nullptr;
}

static void MegaCanvasView_resize(MegaCanvasView *self)
{
    NSRect bounds = [self convertRectToBacking:[self bounds]];
    self->view->resize(bounds.size.width, bounds.size.height);
}

@implementation MegaCanvasView

+ (void)load
{
    @autoreleasepool {
        NSString *shaderPathStr = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Shaders"];
        Mega::shaderPath = strdup([shaderPathStr UTF8String]);
    }
}

- (void)awakeFromNib
{
    NSOpenGLPixelFormatAttribute pfa[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };

    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:pfa];
    if (!pf) {
        NSLog(@"failed to create pixel buffer");
        self.pixelFormat = nil;
        return;
    }
    self.pixelFormat = pf;
    self.wantsBestResolutionOpenGLSurface = YES;
    assert(document);
    view = Mega::View::create(document.canvas);
}

- (void)reshape
{
    [super reshape];
    if (view)
        MegaCanvasView_resize(self);
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];    
    std::string error;
    if (!view->prepare(&error))
        throw std::runtime_error(error);
    MegaCanvasView_resize(self);
}

- (void)drawRect:(NSRect)dirtyRect
{    
    view->render();
    [[self openGLContext] flushBuffer];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    view->moveCenter(round(theEvent.deltaX), round(theEvent.deltaY));
    self.needsDisplay = YES;
}

@end
