//
//  Layer.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Layer_hpp
#define Megacanvas_Layer_hpp

#include <memory>

namespace Mega {
    class Layer {
        struct priv;
        std::unique_ptr<priv> that;
    public:
        Layer();
        ~Layer();
    };
}

#endif
