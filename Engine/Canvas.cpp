//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/Util/Optional.hpp"
#include "Engine/Util/StringSwitch.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Mega {
    //
    // YAML additions
    //
    namespace {
        void operator>>(YAML::Node const &node, Vec &o) {
            std::vector<double> x;
            node >> x;
            if (x.size() != 2)
                throw std::runtime_error("Vec did not have two elements");
            o = Vec(x[0], x[1]);
        }

        template<typename T>
        void operator>>(YAML::Node const &node, Optional<T> &o) {
            T x;
            node >> x;
            o = x;
        }
    }
    
    //
    // internal representations
    //
    constexpr size_t DEFAULT_LOG_SIZE = 7;
    
    template<>
    struct Priv<Canvas> {
        const size_t tileLogSize, tileLogByteSize;
        std::vector<Priv<Layer>> layers;
        std::vector<std::uint8_t> tiles;
        
        Priv(size_t logSize = DEFAULT_LOG_SIZE)
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2)
        { }
    };
    MEGA_PRIV_DTOR(Canvas)

    template<>
    struct Priv<Layer> {
        Vec parallax;
        Vec origin;
        std::vector<size_t> tiles;
        int priority;
        size_t quadtreeDepth;
        
        Priv()
        : parallax(Vec(1.0, 1.0)), origin(Vec(0.0, 0.0)), priority(0), quadtreeDepth(0)
        {}
        
        Priv(Vec parallax, Vec origin, int priority, size_t quadtreeDepth)
        : parallax(parallax), origin(origin), priority(priority), quadtreeDepth(quadtreeDepth)
        {}
    };
    MEGA_PRIV_DTOR(Layer)
    
    //
    // Canvas implementation
    //
    PrivOwner<Canvas> Canvas::create()
    {
        PrivOwner<Canvas> r;
        r.getPriv()->layers.emplace_back();
        return r;
    }
    
    PrivOwner<Canvas> Canvas::load(StringRef path)
    {
        std::string root(path.begin(), path.end());
        std::string metapath = root + "/mega.yaml";
        try {
            std::clog << "parsing " << metapath << '\n';

            std::ifstream meta(metapath);
            
            YAML::Parser metaParser(meta);
            YAML::Node doc;
            
            //
            // parse top-level
            //
            Optional<size_t> version;
            Optional<size_t> logSize;
            Optional<YAML::Node const &> layersNode;
            
            metaParser.GetNextDocument(doc);
            for (auto i = doc.begin(), end = doc.end(); i != end; ++i) {
                std::string key;
                i.first() >> key;
                if (key == "mega") {
                    i.second() >> version;
                } else if (key == "tile-size") {
                    i.second() >> logSize;
                } else if (key == "layers") {
                    layersNode = i.second();
                } else {
                    throw std::runtime_error("unexpected key string " + key);
                }
            }
            
            if (!version)
                throw std::runtime_error("missing 'mega' version key");
            if (*version != 1)
                throw std::runtime_error("'mega' version not supported (must be 1)");
            if (!logSize)
                throw std::runtime_error("missing 'tile-size' key");
            if (!layersNode)
                throw std::runtime_error("missing 'layers' key");
            
            PrivOwner<Canvas> r(*logSize);
            std::vector<Priv<Layer>> &layers = r.getPriv()->layers;
            
            //
            // parse layers
            //
            
            for (YAML::Node const & layerNode : *layersNode) {
                Optional<Vec> parallax;
                Optional<Vec> origin;
                Optional<int> priority;
                Optional<size_t> quadtreeDepth;
                Optional<YAML::Node const &> tilesNode;
                
                for (auto i = layerNode.begin(), end = layerNode.end(); i != end; ++i) {
                    std::string key;
                    i.first() >> key;
                    if (key == "parallax") {
                        i.second() >> parallax;
                    } else if (key == "origin") {
                        i.second() >> origin;
                    } else if (key == "priority") {
                        i.second() >> priority;
                    } else if (key == "size") {
                        i.second() >> quadtreeDepth;
                    } else if (key == "tiles") {
                        tilesNode = i.second();
                    }
                }
                
                if (!parallax)
                    throw std::runtime_error("layer missing 'parallax' key");
                if (!origin)
                    throw std::runtime_error("layer missing 'origin' key");
                if (!priority)
                    throw std::runtime_error("layer missing 'priority' key");
                if (!quadtreeDepth)
                    throw std::runtime_error("layer missing 'size' key");
                if (!tilesNode)
                    throw std::runtime_error("layer missing 'tiles' key");
                
                layers.emplace_back(*parallax, *origin, *priority, *quadtreeDepth);
                *tilesNode >> layers.back().tiles;
                
                if (layers.back().tiles.size() != (1 << (*quadtreeDepth << 1)))
                    throw std::runtime_error("layer has wrong number of tiles for given size");
            }
            
            //
            // XXX load tiles
            //
            
            return r;
        } catch (std::exception const &ex) {
            std::clog << "error while loading " << root << ": " << ex.what() << '\n';
            return PrivOwner<Canvas>();
        } catch (...) {
            std::clog << "unknown exception while loading " << root << '\n';
            return PrivOwner<Canvas>();
        }
    }
    
    MEGA_PRIV_GETTER(Canvas, tileLogSize, size_t)
    MEGA_PRIV_GETTER(Canvas, layers, PrivArrayRef<Layer>)

    size_t Canvas::tileSize()
    {
        return 1 << that->tileLogSize;
    }
    
    size_t Canvas::tileArea()
    {
        return 1 << (that->tileLogSize << 1);
    }
    
    size_t Canvas::tileCount()
    {
        size_t size = that->tiles.size(), byteSize = that->tileLogByteSize;
        assert((size & ((1 << byteSize) - 1)) == 0);
        return size >> byteSize;
    }
    
    ArrayRef<std::uint8_t> Canvas::tile(size_t i)
    {
        assert(i < this->tileCount());
        std::uint8_t *begin = that->tiles.data();
        size_t byteSize = that->tileLogByteSize;
        return makeArrayRef(begin + (i << byteSize), begin + ((i+1) << byteSize));
    }
    
    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER_SETTER(Layer, priority, int)
}