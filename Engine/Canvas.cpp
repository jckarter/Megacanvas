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
    constexpr size_t DEFAULT_LOG_SIZE = 7;

    template<>
    struct Priv<Canvas> {
        const size_t tileLogSize, tileLogByteSize;
        std::vector<Priv<Layer>> layers;
        std::vector<std::uint8_t> tiles;

        Priv(size_t logSize = DEFAULT_LOG_SIZE)
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2)
        { }

        Priv(size_t logSize, std::vector<Priv<Layer>> &&layers)
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2), layers(layers)
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

        Priv(Vec parallax, Vec origin, int priority, size_t quadtreeDepth, std::vector<size_t> &&tiles)
        : parallax(parallax), origin(origin), priority(priority), quadtreeDepth(quadtreeDepth), tiles(tiles)
        {}
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
                size_t i = 0;
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
        Optional<size_t> version;
        Optional<size_t> tileCount;
        Optional<size_t> logSize;

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

            size_t largestTileIndex = 0;

            SmallString<16> scratch;
            for (auto &kv : *metaRoot) {
                auto keyNode = dyn_cast<yaml::ScalarNode>(kv.getKey());
                _MEGA_LOAD_ERROR_IF(!keyNode,
                                    metaPath << ": root node has non-string key");
                StringRef key = keyNode->getValue(scratch);

                auto valueNode = kv.getValue();
                if (key == "mega") {
                    _MEGA_LOAD_ERROR_IF(!(version = intFromNode<size_t>(valueNode, scratch)),
                                        metaPath << ": 'mega' value is not an integer");
                } else if (key == "tile-size") {
                    _MEGA_LOAD_ERROR_IF(!(logSize = intFromNode<size_t>(valueNode, scratch)),
                                        metaPath << ": 'tile-size' value is not an integer");
                } else if (key == "tile-count") {
                    _MEGA_LOAD_ERROR_IF(!(tileCount = intFromNode<size_t>(valueNode, scratch)),
                                        metaPath << ": 'tile-count' value is not an integer");
                } else if (key == "layers") {
                    auto layersNode = dyn_cast<yaml::SequenceNode>(valueNode);
                    _MEGA_LOAD_ERROR_IF(!layersNode,
                                        metaPath << ": 'layers' value is not a sequence");
                    size_t layerI = 0;
                    for (auto &layerNode : *layersNode) {
                        auto layerMap = dyn_cast<yaml::MappingNode>(&layerNode);
                        _MEGA_LOAD_ERROR_IF(!layerMap,
                                            metaPath << ": layer " << layerI << ": node is not a mapping node");

                        Optional<Vec> parallax;
                        Optional<Vec> origin;
                        Optional<int> priority;
                        Optional<size_t> quadtreeDepth;
                        vector<size_t> tiles;

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
                                _MEGA_LOAD_ERROR_IF(!(quadtreeDepth = intFromNode<size_t>(layerValueNode, scratch)),
                                                    metaPath << ": layer " << layerI << ": 'size' value is not a vec");
                            } else if (layerKey == "tiles") {
                                auto tilesNode = dyn_cast<yaml::SequenceNode>(layerValueNode);
                                _MEGA_LOAD_ERROR_IF(!tilesNode,
                                                    metaPath << ": layer " << layerI << ": 'tiles' value is not a sequence");
                                for (auto &tileNode : *tilesNode) {
                                    Optional<size_t> tile = intFromNode<size_t>(&tileNode, scratch);
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
            for (size_t tile = 0; tile < *tileCount; ++tile) {
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

    Array2DRef<std::uint8_t> Canvas::tiles()
    {
        return Array2DRef<std::uint8_t>(that->tiles, 1 << that->tileLogByteSize);
    }

    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER_SETTER(Layer, priority, int)
    MEGA_PRIV_GETTER(Layer, origin, Vec)
}