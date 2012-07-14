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
#include "Engine/Util/OpaqueIterator.hpp"
#include "Engine/Util/Priv.hpp"
#include <memory>

namespace Mega {
    struct Layer;
    struct LayerCursor;

    struct Canvas : HasPriv<Canvas> {
        MEGA_PRIV_CTORS(Canvas)

        static Owner<Canvas> create();
        static Owner<Canvas> load(llvm::StringRef path, std::string *outError);

        PrivArrayRef<Layer> layers();

        std::size_t tileLogSize();
        std::size_t tileSize();
        std::size_t tileArea();
        std::size_t tileByteSize();
        std::size_t tileCount();

        Array2DRef<std::uint8_t> tiles();
    };
}

#endif
