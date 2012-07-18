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
    [self.openGLContext makeCurrentContext];
    MegaCanvasView_resize(self);
}

- (void)prepareOpenGL
{
    std::string error;
    if (!view->prepare(&error))
        throw std::runtime_error(error);
    MegaCanvasView_resize(self);
}

- (void)drawRect:(NSRect)dirtyRect
{
    auto context = self.openGLContext;
    [context makeCurrentContext];
    view->render();
    [context flushBuffer];
}

- (void)scrollWheel:(NSEvent *)event
{
    [self.openGLContext makeCurrentContext];
    double zoom = view->zoom();
    view->moveCenter(-event.deltaX/zoom, event.deltaY/zoom);
    self.needsDisplay = YES;
}

- (void)magnifyWithEvent:(NSEvent *)event
{
    [self.openGLContext makeCurrentContext];
    view->moveZoom(event.magnification);
    self.needsDisplay = YES;
}

- (void)endGestureWithEvent:(NSEvent *)event
{
//    NSLog(@"gesture stop %lu", event.type);
}

- (void)dealloc
{
    [self.openGLContext makeCurrentContext];
    view.reset();
}

@end
