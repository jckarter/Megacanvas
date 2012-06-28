//
//  Canvas.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_megacanvas_h
#define Megacanvas_megacanvas_h

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include "Engine/Util/Priv.hpp"
#include <memory>

namespace Mega {
    struct Layer;
    
    struct Canvas : HasPriv<Canvas> {        
        MEGA_PRIV_CTORS(Canvas)
        
        static PrivOwner<Canvas> create();
        static PrivOwner<Canvas> load(llvm::StringRef path, std::string *outError);
        
        PrivArrayRef<Layer> layers();
        
        size_t tileLogSize();
        size_t tileSize();
        size_t tileArea();
        size_t tileByteSize();
        size_t tileCount();
        llvm::MutableArrayRef<std::uint8_t> tile(size_t i);
    };
}

#endif
