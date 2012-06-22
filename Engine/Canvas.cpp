//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/Tile.hpp"
#include <vector>

namespace Mega {
    //
    // internal representations
    //
    template<>
    struct Priv<Canvas> {
        std::vector<Priv<Layer>> layers;
        std::vector<Priv<Tile>> tiles;
    };
    MEGA_PRIV_DTOR(Canvas)

    template<typename T>
    struct Quadtree {};
    
    template<>
    struct Priv<Layer> {
        double parallax;
        int priority;
        Point origin;
        Quadtree<size_t> tiles;
    };
    MEGA_PRIV_DTOR(Layer)
    
    constexpr size_t tileSize(size_t logSize) { return 1 << logSize; }
    constexpr size_t tileArea(size_t logSize) { auto s = tileSize(logSize); return s*s; }
    
    template<>
    struct Priv<Tile> {
        std::unique_ptr<std::uint8_t[]> pixels;
        size_t logSize;
        
        size_t size() const { return tileSize(logSize); }
        size_t area() const { return tileArea(logSize); }
        
        Priv(size_t logSize)
        : pixels(new std::uint8_t[tileArea(logSize)]), logSize(logSize) {}
        
        // takes ownership of pixels
        Priv(std::uint8_t *pixels, size_t logSize)
        : pixels(pixels), logSize(logSize) {}
    };
    MEGA_PRIV_DTOR(Tile)

    //
    // Canvas implementation
    //
    PrivOwner<Canvas> Canvas::create()
    {
        return PrivOwner<Canvas>();
    }
    
    ArrayRef<Priv<Layer>> Canvas::layers()
    {
        return that->layers;
    }
    
    ArrayRef<Priv<Tile>> Canvas::tiles()
    {
        return that->tiles;
    }
    
    Layer Canvas::layer(size_t i)
    {
        return that->layers[i];
    }
    
    Tile Canvas::tile(size_t i)
    {
        return that->tiles[i];
    }
    
    //
    // Layer implementation
    //
    
    //
    // Tile implementation
    //
    ArrayRef<std::uint8_t> Tile::pixels()
    {
        return makeArrayRef(that->pixels.get(), that->area());
    }
}