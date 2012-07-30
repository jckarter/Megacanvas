//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/Util/StructMeta.hpp"
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/system_error.h>
#include <functional>
#include <vector>

//fixme mac-specific
#include <CoreFoundation/CoreFoundation.h>

namespace Mega {
    //
    // internal representations
    //
    constexpr std::size_t DEFAULT_LOG_SIZE = 7;
    
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

    template<>
    struct Priv<Canvas> {
        const std::size_t tileLogSize, tileLogByteSize;
        std::vector<Priv<Layer>> layers;
        std::string tilesPath;
        std::size_t tileCount;
        
        Priv(std::size_t logSize = DEFAULT_LOG_SIZE, llvm::StringRef tilesPath = "")
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2),
        tilesPath(tilesPath), tileCount(0)
        { }

        Priv(std::size_t logSize, std::vector<Priv<Layer>> &&layers,
             llvm::StringRef tilesPath, std::size_t tileCount)
        :
        tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2), layers(layers),
        tilesPath(tilesPath), tileCount(tileCount)
        { }
        
        void makeTilePath(std::size_t i, llvm::SmallVectorImpl<char> *outPath)
        {
            outPath->clear();
            llvm::raw_svector_ostream os(*outPath);
            using namespace llvm::sys;
            os << $.tilesPath << '/' << i << ".rgba";
            os.flush();
        }
    };
    MEGA_PRIV_DTOR(Canvas)

    template<>
    struct Priv<Layer> {
        Vec parallax;
        Vec origin;
        std::vector<Layer::tile_t> tiles;
        std::size_t quadtreeDepth;

        Priv()
        : parallax{1.0, 1.0}, origin{0.0, 0.0}, quadtreeDepth(0)
        {}

        Priv(Vec parallax, Vec origin, std::size_t quadtreeDepth, std::vector<Layer::tile_t> &&tiles)
        : parallax(parallax), origin(origin), quadtreeDepth(quadtreeDepth), tiles(tiles)
        {}
        
        Layer::tile_t *segmentCorner(std::ptrdiff_t quadrantSize,
                                     std::ptrdiff_t x, std::ptrdiff_t y)
        {
            using namespace std;
            size_t logRadius = $.quadtreeDepth - 1;
            size_t radius = 1 << logRadius;
            size_t nodeSize = 1 << (logRadius << 1);
            size_t xa = x*quadrantSize + radius, ya = y*quadrantSize + radius;
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
                return Vec{components[0], components[1]};
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
                        _MEGA_LOAD_ERROR_IF(!quadtreeDepth,
                                            metaPath << ": layer " << layerI << ": layer missing 'size' key");
                        _MEGA_LOAD_ERROR_IF(tiles.size() != (1 << (*quadtreeDepth << 1)),
                                            metaPath << ": layer " << layerI << ": layer has 'size' value " << *quadtreeDepth << " (tile count " << (1 << (*quadtreeDepth << 1)) << ") but 'tiles' value only lists " << tiles.size() << "tiles");

                        layers.emplace_back(*parallax, *origin, *quadtreeDepth, move(tiles));

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

            result = createOwner<Canvas>(*logSize, move(layers), path, *tileCount);
        }
        /*
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
         */

        return result;

#undef _MEGA_LOAD_ERROR_IF
error:
        assert(!outError->empty());
        return Owner<Canvas>();
    }

    MEGA_PRIV_GETTER(Canvas, tileLogSize, std::size_t)
    MEGA_PRIV_GETTER(Canvas, layers, PrivArrayRef<Layer>)
    MEGA_PRIV_GETTER(Canvas, tileCount, std::size_t)

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
    
    bool Canvas::verifyTiles(std::string *outError)
    {
        using namespace llvm;
        using namespace llvm::sys;
        raw_string_ostream errors(*outError);
        SmallString<260> path;
        bool ok = true;
        for (std::size_t i = 1; i <= $.tileCount; ++i) {
            $.makeTilePath(i, &path);
            bool isRegular;
            error_code code = fs::is_regular_file(path.str(), isRegular);
            if (code) {
                errors << path << ": " << code.message() << "\n";
                ok = false;
            } else if (!isRegular) {
                errors << path << ": not a regular file";
                ok = false;
            }
        }
        return ok;
    }
    
    MappedFile
    Canvas::loadTile(std::size_t index, std::string *outError)
    {
        assert(index >= 1 && index <= $.tileCount);
        
        llvm::SmallString<260> path;
        $.makeTilePath(index, &path);
        
        MappedFile f(path, outError);
        f.sequential();
        return f;
    }
    
    struct CallbackInfo {
        std::function<void (bool, const std::string &)> userCallback;
        std::uint8_t *ptr;
        std::size_t size;
    };
    
    static std::string streamErrorString(CFReadStreamRef stream)
    {
        CFErrorRef error = CFReadStreamCopyError(stream);
        MEGA_FINALLY({ CFRelease(error); });
        CFStringRef desc = CFErrorCopyDescription(error);
        MEGA_FINALLY({ CFRelease(desc); });
        
        std::unique_ptr<char[]> buf(new char[CFStringGetLength(desc)*4+1]);
        bool ok = CFStringGetCString(desc, buf.get(), CFStringGetLength(desc)*4+1,
                                     kCFStringEncodingUTF8);
        assert(ok);
        
        return std::string(buf.get());
    }
    
    void loadTileIntoCallback(CFReadStreamRef stream,
                              CFStreamEventType eventType,
                              void *clientCallBackInfo)
    {
        auto callback = reinterpret_cast<CallbackInfo*>(clientCallBackInfo);
        switch (eventType) {
            case kCFStreamEventHasBytesAvailable: {
                CFIndex read = CFReadStreamRead(stream, callback->ptr, callback->size);
                if (read == -1)
                    goto error;
                else if (read >= callback->size)
                    goto finished;
                else if (read == 0)
                    goto unexpectedEnd;
                else {
                    callback->ptr += read;
                    callback->size -= read;
                }
                return;
            }
            case kCFStreamEventEndEncountered:
                goto unexpectedEnd;
            case kCFStreamEventErrorOccurred:
                goto error;
            default:
                assert(false);
                return;
        }
        
    error:
        callback->userCallback(false, streamErrorString(stream));
        goto cleanup;

    finished:
        callback->userCallback(true, "");
        goto cleanup;
        
    unexpectedEnd:
        {
            std::string error;
            llvm::raw_string_ostream errors(error);
            errors << "reached end of file but expected " << callback->size << " more bytes";
            callback->userCallback(false, errors.str());
            goto cleanup;
        }

    cleanup:
        CFReadStreamClose(stream);
        CFRelease(stream);
        delete callback;
    }
    
    void
    Canvas::loadTileInto(std::size_t index, llvm::MutableArrayRef<std::uint8_t> outBuffer,
                         std::function<void (bool, const std::string &)> callback)
    {
        assert(index >= 1 && index <= $.tileCount);
        assert(outBuffer.size() >= $$.tileByteSize());
        llvm::SmallString<260> path;
        $.makeTilePath(index, &path);
        
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL,
                                                               reinterpret_cast<UInt8 const*>(path.c_str()),
                                                               path.size(),
                                                               false);
        MEGA_FINALLY({ CFRelease(url); });
        CFReadStreamRef stream = CFReadStreamCreateWithFile(NULL, url);
        
        auto callbackBuf = new CallbackInfo{
            std::move(callback),
            outBuffer.begin(), outBuffer.size()
        };
        
        CFStreamClientContext context{0, callbackBuf, nullptr, nullptr, nullptr};
        CFReadStreamSetClient(stream,
                              kCFStreamEventHasBytesAvailable
                              | kCFStreamEventEndEncountered | kCFStreamEventErrorOccurred,
                              loadTileIntoCallback,
                              &context);
        CFReadStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        
        if (!CFReadStreamOpen(stream)) {
            callbackBuf->userCallback(false, streamErrorString(stream));
            CFRelease(stream);
            delete callbackBuf;
            return;
        }
    }
    
    void Canvas::wasMoved(llvm::StringRef newPath)
    {
        $.tilesPath = newPath;
    }

    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER_SETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER(Layer, origin, Vec)
    
    namespace {
        bool isPowerOfTwo(std::size_t x) 
        {
            return (x & (x - 1)) == 0 && x != 0;
        }
    }
    
    Layer::SegmentRef
    Layer::segment(std::size_t segmentSize, std::ptrdiff_t x, std::ptrdiff_t y)
    {
        assert(isPowerOfTwo(segmentSize) && segmentSize*segmentSize <= PTRDIFF_MAX);
        using namespace std;
        using namespace llvm;
        size_t quadtreeDepth = $.quadtreeDepth;
        if (quadtreeDepth == 0) {
            return {ArrayRef<tile_t>(), 0};
        }
        size_t radius = 1 << (quadtreeDepth - 1);
        size_t nodeSize = 1 << ((quadtreeDepth - 1) << 1);
        tile_t *tiles = $.tiles.data();
        if (radius < segmentSize) {
            if ((x != -1 && x != 0) || (y != -1 && y != 0))
                return {ArrayRef<tile_t>(), 0};

            size_t node = (x+1) | ((y+1)<<1);
            size_t offset = 0;
            while (segmentSize > radius) {
                offset = (offset << 2) | 1;
                segmentSize >>= 1;
            }
            while (segmentSize > 1) {
                offset <<= 2;
                segmentSize >>= 1;
            }
            return {makeArrayRef(&tiles[node*nodeSize], nodeSize), (3-node)*offset};
        } else {
            ptrdiff_t segmentRadius = radius/segmentSize;
            if (x < -segmentRadius || x >= segmentRadius
                || y < -segmentRadius || y >= segmentRadius) {
                return {ArrayRef<tile_t>(), 0};
            }
            Layer::tile_t *segment = $.segmentCorner(segmentSize, x, y);
            return {makeArrayRef(segment, segmentSize*segmentSize), 0};
        }
    }
}