//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <functional>
#include <stdexcept>
#include <vector>

namespace Mega {
    //
    // internal representations
    //
    constexpr std::size_t DEFAULT_LOG_SIZE = 7;

    template<>
    struct Priv<Canvas> {
        const std::size_t tileLogSize, tileLogByteSize;
        std::vector<Priv<Layer>> layers;
        std::vector<std::uint8_t> tiles;

        Priv(std::size_t logSize = DEFAULT_LOG_SIZE)
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2)
        { }

        Priv(std::size_t logSize, std::vector<Priv<Layer>> &&layers)
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2), layers(layers)
        { }

        void resizeTiles(std::size_t count) { tiles.resize(count << $.tileLogByteSize); }
        llvm::MutableArrayRef<std::uint8_t> tile(std::size_t i)
        {
            std::size_t byteSize = $.tileLogByteSize;
            assert((i << byteSize) <= $.tiles.size());
            std::uint8_t *begin = $.tiles.data() + ((i-1) << byteSize);
            return llvm::MutableArrayRef<std::uint8_t>(begin, 1 << byteSize);
        }
    };
    MEGA_PRIV_DTOR(Canvas)

    template<>
    struct Priv<Layer> {
        Vec parallax;
        Vec origin;
        std::vector<Layer::tile_t> tiles;
        int priority;
        std::size_t quadtreeDepth;

        Priv()
        : parallax(Vec(1.0, 1.0)), origin(Vec(0.0, 0.0)), priority(0), quadtreeDepth(0)
        {}

        Priv(Vec parallax, Vec origin, int priority, std::size_t quadtreeDepth, std::vector<Layer::tile_t> &&tiles)
        : parallax(parallax), origin(origin), priority(priority), quadtreeDepth(quadtreeDepth), tiles(tiles)
        {}
        
        Layer::tile_t *segmentCorner(std::ptrdiff_t x, std::ptrdiff_t y,
                                     std::ptrdiff_t segmentSize)
        {
            using namespace std;
            size_t logRadius = $.quadtreeDepth - 1;
            size_t radius = 1 << logRadius;
            size_t nodeSize = 1 << (logRadius << 1);
            size_t xa = x*segmentSize + radius, ya = y*segmentSize + radius;
            assert(xa >= 0 && xa < radius*2 && ya >= 0 && ya < radius*2);
            Layer::tile_t *corner = tiles.data();
            
            while (xa != 0 || ya != 0) {
                assert(nodeSize > 0 && radius > 0);
                if (xa >= radius) {
                    corner += nodeSize;
                    xa -= radius;
                }
                if (ya >= radius) {
                    corner += 2*nodeSize;
                    ya -= radius;
                }
                nodeSize >>= 2;
                radius >>= 1;
            }
            assert(xa == 0 && ya == 0);
            
            return corner;
        }
    };
    MEGA_PRIV_DTOR(Layer)

    //
    // Canvas implementation
    //
    Owner<Canvas> Canvas::create()
    {
        auto r = createOwner<Canvas>();
        r.priv().layers.emplace_back();
        return r;
    }

    namespace {
        template<typename T>
        llvm::Optional<T> intFromNode(llvm::yaml::Node *node, llvm::SmallVectorImpl<char> &scratch)
        {
            using namespace llvm;
            auto sNode = dyn_cast<yaml::ScalarNode>(node);
            if (sNode) {
                T result;

                if (!sNode->getValue(scratch).getAsInteger(10, result))
                    return result;
            }
            return Optional<T>();
        }

        llvm::Optional<Vec> vecFromNode(llvm::yaml::Node *node, llvm::SmallVectorImpl<char> &scratch)
        {
            using namespace llvm;
            auto seqNode = dyn_cast<yaml::SequenceNode>(node);
            if (seqNode) {
                double components[2];
                std::size_t i = 0;
                for (auto &subnode : *seqNode) {
                    if (i >= 2)
                        return Optional<Vec>();
                    auto sNode = dyn_cast<yaml::ScalarNode>(&subnode);
                    if (!sNode)
                        return Optional<Vec>();
                    SmallString<16> value = sNode->getValue(scratch);
                    char *end;
                    components[i] = strtod(value.c_str(), &end);
                    if (*end != '\0')
                        return Optional<Vec>();
                    ++i;
                }
                return Vec(components[0], components[1]);
            }
            return Optional<Vec>();
        }
    }

    Owner<Canvas> Canvas::load(llvm::StringRef path, std::string *outError)
    {
        using namespace std;
        using namespace llvm;
        using namespace llvm::sys;
        assert(outError);
        raw_string_ostream errors(*outError);
#define _MEGA_LOAD_ERROR_IF(cond, inserts) \
    if (cond) { errors << inserts; errors.str(); goto error; } else

        Owner<Canvas> result;
        Optional<std::size_t> version;
        Optional<std::size_t> tileCount;
        Optional<std::size_t> logSize;

        {
            SmallString<256> metaPath(path);
            path::append(metaPath, "mega.yaml");
            SourceMgr metaSM;
            OwningPtr<MemoryBuffer> metaBuf;
            auto errorCode = MemoryBuffer::getFile(metaPath, metaBuf);
            _MEGA_LOAD_ERROR_IF(errorCode, metaPath << ": unable to read file: " << errorCode.message());

            yaml::Stream metaStream(metaBuf->getBuffer(), metaSM);
            _MEGA_LOAD_ERROR_IF(metaStream.failed(),
                                metaPath << ": unable to parse yaml");
            yaml::document_iterator metaDoc = metaStream.begin();
            _MEGA_LOAD_ERROR_IF(metaDoc == metaStream.end(),
                                metaPath << ": no yaml documents in stream");

            //
            // parse metadata
            //
            auto metaRoot = dyn_cast<yaml::MappingNode>(metaDoc->getRoot());
            _MEGA_LOAD_ERROR_IF(!metaRoot,
                                metaPath << ": root node is not a mapping node");

            vector<Priv<Layer>> layers;

            Layer::tile_t largestTileIndex = 0;

            SmallString<16> scratch;
            for (auto &kv : *metaRoot) {
                auto keyNode = dyn_cast<yaml::ScalarNode>(kv.getKey());
                _MEGA_LOAD_ERROR_IF(!keyNode,
                                    metaPath << ": root node has non-string key");
                StringRef key = keyNode->getValue(scratch);

                auto valueNode = kv.getValue();
                if (key == "mega") {
                    _MEGA_LOAD_ERROR_IF(!(version = intFromNode<std::size_t>(valueNode, scratch)),
                                        metaPath << ": 'mega' value is not an integer");
                } else if (key == "tile-size") {
                    _MEGA_LOAD_ERROR_IF(!(logSize = intFromNode<std::size_t>(valueNode, scratch)),
                                        metaPath << ": 'tile-size' value is not an integer");
                } else if (key == "tile-count") {
                    _MEGA_LOAD_ERROR_IF(!(tileCount = intFromNode<std::size_t>(valueNode, scratch)),
                                        metaPath << ": 'tile-count' value is not an integer");
                } else if (key == "layers") {
                    auto layersNode = dyn_cast<yaml::SequenceNode>(valueNode);
                    _MEGA_LOAD_ERROR_IF(!layersNode,
                                        metaPath << ": 'layers' value is not a sequence");
                    std::size_t layerI = 0;
                    for (auto &layerNode : *layersNode) {
                        auto layerMap = dyn_cast<yaml::MappingNode>(&layerNode);
                        _MEGA_LOAD_ERROR_IF(!layerMap,
                                            metaPath << ": layer " << layerI << ": node is not a mapping node");

                        Optional<Vec> parallax;
                        Optional<Vec> origin;
                        Optional<int> priority;
                        Optional<std::size_t> quadtreeDepth;
                        vector<Layer::tile_t> tiles;

                        for (auto &layerKV : *layerMap) {
                            auto layerKeyNode = dyn_cast<yaml::ScalarNode>(layerKV.getKey());
                            _MEGA_LOAD_ERROR_IF(!layerKeyNode,
                                                metaPath << ": layer " << layerI << ": node has non-string key");
                            StringRef layerKey = layerKeyNode->getValue(scratch);

                            auto layerValueNode = layerKV.getValue();
                            if (layerKey == "parallax") {
                                _MEGA_LOAD_ERROR_IF(!(parallax = vecFromNode(layerValueNode, scratch)),
                                                    metaPath << ": layer " << layerI << ": 'parallax' value is not a vec");
                            } else if (layerKey == "origin") {
                                _MEGA_LOAD_ERROR_IF(!(origin = vecFromNode(layerValueNode, scratch)),
                                                    metaPath << ": layer " << layerI << ": 'origin' value is not a vec");
                            } else if (layerKey == "priority") {
                                _MEGA_LOAD_ERROR_IF(!(priority = intFromNode<int>(layerValueNode, scratch)),
                                                    metaPath << ": layer " << layerI << ": 'priority' value is not an integer");
                            } else if (layerKey == "size") {
                                _MEGA_LOAD_ERROR_IF(!(quadtreeDepth = intFromNode<std::size_t>(layerValueNode, scratch)),
                                                    metaPath << ": layer " << layerI << ": 'size' value is not a vec");
                            } else if (layerKey == "tiles") {
                                auto tilesNode = dyn_cast<yaml::SequenceNode>(layerValueNode);
                                _MEGA_LOAD_ERROR_IF(!tilesNode,
                                                    metaPath << ": layer " << layerI << ": 'tiles' value is not a sequence");
                                for (auto &tileNode : *tilesNode) {
                                    Optional<Layer::tile_t> tile = intFromNode<Layer::tile_t>(&tileNode, scratch);
                                    _MEGA_LOAD_ERROR_IF(!tile,
                                                        metaPath << ": layer " << layerI << ": 'tiles' sequence contains non-integer values");
                                    largestTileIndex = max(largestTileIndex, *tile);
                                    tiles.push_back(*tile);
                                }
                            } else {
                                _MEGA_LOAD_ERROR_IF(true,
                                                    metaPath << ": layer " << layerI << ": unexpected key '" << layerKey << "'");
                            }
                        }

                        _MEGA_LOAD_ERROR_IF(!parallax,
                                            metaPath << ": layer " << layerI << ": missing 'parallax' key");
                        _MEGA_LOAD_ERROR_IF(!origin,
                                            metaPath << ": layer " << layerI << ": layer missing 'origin' key");
                        _MEGA_LOAD_ERROR_IF(!priority,
                                            metaPath << ": layer " << layerI << ": layer missing 'priority' key");
                        _MEGA_LOAD_ERROR_IF(!quadtreeDepth,
                                            metaPath << ": layer " << layerI << ": layer missing 'size' key");
                        _MEGA_LOAD_ERROR_IF(tiles.size() != (1 << (*quadtreeDepth << 1)),
                                            metaPath << ": layer " << layerI << ": layer has 'size' value " << *quadtreeDepth << " (tile count " << (1 << (*quadtreeDepth << 1)) << ") but 'tiles' value only lists " << tiles.size() << "tiles");

                        layers.emplace_back(*parallax, *origin, *priority, *quadtreeDepth, move(tiles));

                        ++layerI;
                    }
                } else
                    _MEGA_LOAD_ERROR_IF(true, metaPath << ": unexpected key '" << key << "'");
            }

            _MEGA_LOAD_ERROR_IF(!version, metaPath << ": missing 'mega' key");
            _MEGA_LOAD_ERROR_IF(*version != 1, metaPath << ": 'mega' version not supported (must be 1)");
            _MEGA_LOAD_ERROR_IF(!logSize, metaPath << ": missing 'tile-size' key");
            _MEGA_LOAD_ERROR_IF(!tileCount, metaPath << ": missing 'tile-count' key");
            _MEGA_LOAD_ERROR_IF(layers.empty(), metaPath << ": must be at least one layer");

            result = createOwner<Canvas>(*logSize, move(layers));
        }

        {
            //
            // load tiles
            //
            result.priv().resizeTiles(*tileCount);
            std::string tileFilename;
            for (std::size_t tile = 1; tile <= *tileCount; ++tile) {
                tileFilename.clear();
                raw_string_ostream paths(tileFilename);
                paths << tile << ".rgba";
                SmallString<256> tilePath(path);
                path::append(tilePath, paths.str());

                OwningPtr<MemoryBuffer> tileBuf;
                auto errorCode = MemoryBuffer::getFile(tilePath.c_str(), tileBuf);
                _MEGA_LOAD_ERROR_IF(errorCode,
                                    tilePath << ": unable to read file: " << errorCode.message());

                MutableArrayRef<uint8_t> destTileData = result.priv().tile(tile);

                _MEGA_LOAD_ERROR_IF(destTileData.size() != tileBuf->getBufferSize(),
                                    tilePath << ": expected " << destTileData.size() << " bytes, but file has " << tileBuf->getBufferSize() << "bytes");

                memcpy(reinterpret_cast<void*>(destTileData.begin()),
                       reinterpret_cast<const void*>(tileBuf->getBufferStart()),
                       destTileData.size());
            }
        }

        return result;

#undef _MEGA_LOAD_ERROR_IF
error:
        assert(!outError->empty());
        return Owner<Canvas>();
    }

    MEGA_PRIV_GETTER(Canvas, tileLogSize, std::size_t)
    MEGA_PRIV_GETTER(Canvas, layers, PrivArrayRef<Layer>)

    std::size_t Canvas::tileSize()
    {
        return 1 << $.tileLogSize;
    }

    std::size_t Canvas::tileArea()
    {
        return 1 << ($.tileLogSize << 1);
    }

    std::size_t Canvas::tileByteSize()
    {
        return 1 << $.tileLogByteSize;
    }

    Array2DRef<std::uint8_t> Canvas::tiles()
    {
        return Array2DRef<std::uint8_t>($.tiles, 1 << $.tileLogByteSize);
    }

    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER_SETTER(Layer, priority, int)
    MEGA_PRIV_GETTER(Layer, origin, Vec)
    
    namespace {
        void zeroSegment(Layer::tile_t *outBuffer, std::size_t segmentSize)
        {
            memset(reinterpret_cast<void*>(outBuffer), 
                   0, sizeof(Layer::tile_t)*segmentSize*segmentSize);            
        }
        
        bool isPowerOfTwo(std::size_t x) 
        {
            return (x & (x - 1)) == 0 && x != 0;
        }
        
        size_t swizzle(std::size_t x, std::size_t y)
        {
            assert(x <= 0xFFFF && y <= 0xFFFF);
            static const unsigned int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
            static const unsigned int S[] = {1, 2, 4, 8};
            
            x = (x | (x << S[3])) & B[3];
            x = (x | (x << S[2])) & B[2];
            x = (x | (x << S[1])) & B[1];
            x = (x | (x << S[0])) & B[0];
            
            y = (y | (y << S[3])) & B[3];
            y = (y | (y << S[2])) & B[2];
            y = (y | (y << S[1])) & B[1];
            y = (y | (y << S[0])) & B[0];
            
            return x | (y << 1);
        }
    }
    
    void Layer::getSegment(std::ptrdiff_t x, std::ptrdiff_t y,
                           Layer::tile_t *outBuffer, std::size_t segmentSize)
    {
        assert(isPowerOfTwo(segmentSize) && segmentSize <= PTRDIFF_MAX);
        using namespace std;
        ptrdiff_t ssegmentSize = ptrdiff_t(segmentSize);
        size_t quadtreeDepth = $.quadtreeDepth;
        if (quadtreeDepth == 0) {
            zeroSegment(outBuffer, ssegmentSize);
            return;
        }
        size_t radius = 1 << (quadtreeDepth - 1);
        size_t nodeSize = 1 << ((quadtreeDepth - 1) << 1);
        if (radius < ssegmentSize) {
            zeroSegment(outBuffer, ssegmentSize);
            Layer::tile_t *segment = $.tiles.data();
            Layer::tile_t *out = outBuffer;
            if (x == -1 && y == -1) {
                out += (segmentSize - radius)*(segmentSize + 1);
            } else if (x == 0 && y == -1) {
                segment += nodeSize;
                out += (segmentSize - radius)*segmentSize;
            } else if (x == -1 && y == 0) {
                segment += 2*nodeSize;
                out += segmentSize - radius;
            } else if (x == 0 && y == 0) {
                segment += 3*nodeSize;
            } else
                return;
            for (size_t yi = 0; yi < radius; ++yi)
                for (size_t xi = 0; xi < radius; ++xi)
                    //fixme should swizzle out indexes instead for better locality?
                    out[yi*ssegmentSize + xi] = segment[swizzle(xi, yi)];
        } else {
            ptrdiff_t segmentRadius = radius/ssegmentSize;
            if (x < -segmentRadius || x >= segmentRadius
                || y < -segmentRadius || y >= segmentRadius) {
                zeroSegment(outBuffer, ssegmentSize);
                return;
            }
            Layer::tile_t *segment = $.segmentCorner(x, y, ssegmentSize);
            for (size_t yi = 0; yi < ssegmentSize; ++yi)
                for (size_t xi = 0; xi < ssegmentSize; ++xi)
                    //fixme should swizzle out indexes instead for better locality?
                    *outBuffer++ = segment[swizzle(xi, yi)];
        }
    }
}