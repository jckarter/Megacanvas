//
//  View.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_View_hpp
#define Megacanvas_View_hpp

#include "Engine/Util/Priv.hpp"

namespace Mega {
    class Canvas;
    
    struct View : HasPriv<View> {
        MEGA_PRIV_CTORS(View)
        
        static Owner<View> create(Canvas c);
        
        void prepare();
        void resize(double width, double height);

        void render();
        
        Vec center();
        void center(Vec c);
        
        double zoom();
        void zoom(double z);
        
        Vec viewToCanvas(Vec viewPoint);
        Vec viewToLayer(Vec viewPoint, Layer l);
    };
}

#endif
