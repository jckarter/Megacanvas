//
//  View.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_View_hpp
#define Megacanvas_View_hpp

#include <memory>

namespace Mega {
    class Canvas;
    
    class View {
        struct priv;
        std::unique_ptr<priv> that;
    public:
        View(Canvas c);
        
        void prepare();
        void resize(double width, double height);
        void render();
    };
}

#endif
