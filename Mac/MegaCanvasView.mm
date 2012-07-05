//
//  MegaCanvasView.m
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import "MegaCanvasView.hh"

@implementation MegaCanvasView

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
        [self setPixelFormat:nil];
        return;
    }
    [self setPixelFormat:pf];
}

- (void)reshape
{
    [super reshape];
    view->resize(x, y);
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];
    view->prepare();
    view->resize(x, y);
}

- (void)drawRect:(NSRect)dirtyRect
{
    view->render();
}

@end
