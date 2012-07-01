//
//  MegaCanvasView.h
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "Engine/View.hpp"

@interface MegaCanvasView : NSOpenGLView
{
    Mega::Owner<Mega::View> view;
}

@end
