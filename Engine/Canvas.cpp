//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include <vector>

namespace Mega {
    //
    // internal representations
    //
    template<>
    struct Priv<Canvas> {
        size_t tileSize;
        std::vector<Priv<Layer>> layers;
        std::vector<std::uint8_t> tiles;
        
        Priv() { layers.emplace_back(); }
    };
    MEGA_PRIV_DTOR(Canvas)

    struct Quadtree {
        enum class NodeRef : size_t {};
        typedef std::array<NodeRef, 4> Node;
        std::vector<Node> nodes;
        size_t root;
        size_t logSize;
    };
    
    template<>
    struct Priv<Layer> {
        Vec parallax;
        Vec origin;
        Quadtree tiles;
        int priority;
        
        Priv()
        : parallax(makeVec(1.0, 1.0)), origin(makeVec(0.0, 0.0)), priority(0)
        {}
    };
    MEGA_PRIV_DTOR(Layer)
    
    //
    // Canvas implementation
    //
    constexpr size_t tileSize(size_t logSize) { return 1 << logSize; }
    constexpr size_t tileArea(size_t logSize) { auto s = tileSize(logSize); return s*s; }
    
    PrivOwner<Canvas> Canvas::create()
    {
        return PrivOwner<Canvas>();
    }
    
    MEGA_PRIV_GETTER(Canvas, layers, PrivArrayRef<Layer>)
    MEGA_PRIV_GETTER(Canvas, tiles, PrivArrayRef<Tile>)
        
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
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER_SETTER(Layer, priority, int)
    
    //
    // Tile implementation
    //
    ArrayRef<std::uint8_t> Tile::pixels()
    {
        return makeArrayRef(that->pixels.get(), that->area());
    }
    
    size_t Tile::size()
    {
        return that->size();
    }
}