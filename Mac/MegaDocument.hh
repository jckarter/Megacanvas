//
//  MegaDocument.h
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "Engine/Canvas.hpp"

@class MegaCanvasView;

@interface MegaDocument : NSDocument <NSWindowDelegate>
{
    IBOutlet MegaCanvasView *view;
    Mega::Owner<Mega::Canvas> canvas;
}

@end
