//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/Util/ErrorStream.hpp"
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringSwitch.h>
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
        void operator>>(YAML::Node const &node, llvm::Optional<T> &o) {
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
        
        void resizeTiles(size_t count) { tiles.resize(count << this->tileLogByteSize); }
        llvm::MutableArrayRef<std::uint8_t> tile(size_t i)
        {
            size_t byteSize = this->tileLogByteSize;
            assert(((i+1) << byteSize) <= this->tiles.size());
            std::uint8_t *begin = this->tiles.data() + (i << byteSize);
            return llvm::MutableArrayRef<std::uint8_t>(begin, 1 << byteSize);
        }
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
        auto r = PrivOwner<Canvas>::create();
        r.getPriv()->layers.emplace_back();
        return r;
    }
    
    PrivOwner<Canvas> Canvas::load(llvm::StringRef path, std::string *outError)
    {
        std::string root(path.begin(), path.end());
        std::string metapath = root + "/mega.yaml";
        try {
            std::ifstream meta(metapath);
            if (!meta.good())
                MEGA_RUNTIME_ERROR(metapath << ": unable to open for reading");
            
            YAML::Parser metaParser(meta);
            YAML::Node doc;
            
            //
            // parse top-level
            //
            llvm::Optional<size_t> version;
            llvm::Optional<size_t> tileCount;
            llvm::Optional<size_t> logSize;
            YAML::Node const *layersNode;
            
            if (!metaParser.GetNextDocument(doc))
                MEGA_RUNTIME_ERROR(metapath << ": no yaml documents");
            for (auto i = doc.begin(), end = doc.end(); i != end; ++i) {
                std::string key;
                i.first() >> key;
                if (key == "mega") {
                    i.second() >> version;
                } else if (key == "tile-size") {
                    i.second() >> logSize;
                } else if (key == "tile-count") {
                    i.second() >> tileCount;
                } else if (key == "layers") {
                    layersNode = &i.second();
                } else {
                    MEGA_RUNTIME_ERROR(metapath << ": unexpected key " << key);
                }
            }
            
            if (!version)
                MEGA_RUNTIME_ERROR(metapath << ": missing 'mega' version key");
            if (*version != 1)
                MEGA_RUNTIME_ERROR(metapath << ": 'mega' version not supported (must be 1)");
            if (!logSize)
                MEGA_RUNTIME_ERROR(metapath << ": missing 'tile-size' key");
            if (!tileCount)
                MEGA_RUNTIME_ERROR(metapath << ": missing 'tile-count' key");
            if (!layersNode)
                MEGA_RUNTIME_ERROR(metapath << ": missing 'layers' key");
            
            auto r = PrivOwner<Canvas>::create(*logSize);
            auto priv = r.getPriv();
            auto &layers = priv->layers;
            
            //
            // parse layers
            //
            size_t layer = 0;
            for (YAML::Node const & layerNode : *layersNode) {
                llvm::Optional<Vec> parallax;
                llvm::Optional<Vec> origin;
                llvm::Optional<int> priority;
                llvm::Optional<size_t> quadtreeDepth;
                YAML::Node const *tilesNode;
                
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
                        tilesNode = &i.second();
                    }
                }
                
                if (!parallax)
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": missing 'parallax' key");
                if (!origin)
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": layer missing 'origin' key");
                if (!priority)
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": layer missing 'priority' key");
                if (!quadtreeDepth)
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": layer missing 'size' key");
                if (!tilesNode)
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": layer missing 'tiles' key");
                
                layers.emplace_back(*parallax, *origin, *priority, *quadtreeDepth);
                
                auto &layerTiles = layers.back().tiles;
                for (YAML::Node const & tileNode : *tilesNode) {
                    ptrdiff_t tile;
                    tileNode >> tile;
                    if (tile > PTRDIFF_MAX || tile < -1)
                        MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": references invalid tile index " << tile);
                    size_t utile = tile;
                    if (utile >= *tileCount && utile != -1)
                        MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": references out of bounds tile index " << tile);
                    layerTiles.push_back(utile);
                }
                
                if (layerTiles.size() != (1 << (*quadtreeDepth << 1)))
                    MEGA_RUNTIME_ERROR(metapath << ": layer " << layer << ": layer has wrong number of tiles for given size");
                ++layer;
            }
            meta.close();
            
            //
            // load tiles
            //
            priv->resizeTiles(*tileCount);
            for (size_t tile = 0; tile < *tileCount; ++tile) {
                auto tileData = priv->tile(tile);
                std::string tilepath = MEGA_STRING(root << '/' << tile << ".rgba");
                std::ifstream tilefile(tilepath, std::ios::in | std::ios::binary);
                if (!tilefile.good())
                    MEGA_RUNTIME_ERROR(tilepath << ": unable to open for reading");
                tilefile.read(reinterpret_cast<char*>(tileData.begin()), tileData.size());
                if (tilefile.fail())
                    MEGA_RUNTIME_ERROR(tilepath << ": not enough data for tile");
            }
            
            return r;
        } catch (std::exception const &ex) {
            if (outError) {
                *outError = "exception while loading ";
                *outError += root;
                *outError += ": ";
                *outError += ex.what();
            }
            return PrivOwner<Canvas>();
        } catch (...) {
            if (outError) {
                *outError = "unknown exception while loading ";
                *outError += root;
            }
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
    
    size_t Canvas::tileByteSize()
    {
        return 1 << that->tileLogByteSize;
    }
    
    size_t Canvas::tileCount()
    {
        size_t size = that->tiles.size(), byteSize = that->tileLogByteSize;
        assert((size & ((1 << byteSize) - 1)) == 0);
        return size >> byteSize;
    }
    
    llvm::MutableArrayRef<std::uint8_t> Canvas::tile(size_t i)
    {
        return that->tile(i);
    }
    
    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER_SETTER(Layer, priority, int)
}