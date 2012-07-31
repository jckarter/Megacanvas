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
#include "Engine/Util/MappedFile.hpp"
#include "Engine/Util/OpaqueIterator.hpp"
#include "Engine/Util/Priv.hpp"

namespace Mega {
    struct Layer;

    struct Canvas : HasPriv<Canvas> {
        typedef std::array<std::uint8_t, 4> pixel_t;

        MEGA_PRIV_CTORS(Canvas)

        static Owner<Canvas> create(std::string *outError);
        static Owner<Canvas> load(llvm::StringRef path, std::string *outError);

        PrivArrayRef<Layer> layers();

        std::size_t tileLogSize();
        std::size_t tileSize();
        std::size_t tileArea();
        std::size_t tileByteSize();
        std::size_t tileCount();
        
        bool verifyTiles(std::string *outError);
        void wantTile(std::size_t index);
        bool loadTileInto(std::size_t index,
                          llvm::MutableArrayRef<std::uint8_t> outBuffer,
                          std::string *outError);
        
        void loadTileIntoAsync(std::size_t index,
                               llvm::MutableArrayRef<std::uint8_t> outBuffer,
                               std::function<void(bool ok, std::string const &error)> callback);
        
        void wasMoved(llvm::StringRef newPath);
        
        void blit(llvm::StringRef name,
                  void const *source,
                  size_t sourcePitch, size_t sourceW, size_t sourceH,
                  size_t destLayer, ptrdiff_t destX, ptrdiff_t destY,
                  pixel_t (*blendFunc)(pixel_t src, pixel_t dest));

        bool save(std::string *outError);
        bool saveAs(llvm::StringRef path, std::string *outError);
        
        void undo();
        void redo();
        llvm::StringRef undoName();
        llvm::StringRef redoName();
    };
}

#endif
