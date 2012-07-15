//
//  MegaDocument.h
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "Engine/Canvas.hpp"

@interface MegaDocument : NSDocument <NSWindowDelegate>
{
    Mega::Owner<Mega::Canvas> canvas;
}

@property (readonly) Mega::Canvas canvas;

@end
