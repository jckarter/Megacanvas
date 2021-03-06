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
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <atomic>
#include <cstdio>
#include <functional>
#include <vector>

//fixme mac-specific
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>

namespace Mega {
    using namespace std;
    using namespace llvm;
    
    constexpr size_t DEFAULT_LOG_SIZE = 7;
    
    size_t swizzle(size_t x, size_t y)
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

    //
    // internal representations
    //
    
    struct History;

    template<>
    struct Priv<Canvas> {
        const size_t tileLogSize, tileLogByteSize;
        vector<Priv<Layer>> layers;
        string tilesPath;
        size_t tileCount;
        bool isUniquePath;
        vector<MappedFile> tileCache;
        vector<History> undo, redo;
        
        Priv(string *outError,
             size_t logSize = DEFAULT_LOG_SIZE,
             StringRef tilesPath = "")
        : tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2),
        tilesPath(tilesPath), tileCount(0),
        isUniquePath(false)
        {
            if (tilesPath.empty()) {
                //fixme proper system-aware temp path
                $.tilesPath = "/tmp/megacanvas-XXXXXXXXXXXXXXXX";
                char *x;
                do {
                    x = mkdtemp(const_cast<char*>($.tilesPath.c_str()));
                } while (!x && errno == EINTR);
                if (!x) {
                    *outError = strerror(errno);
                    return;
                }
                $.isUniquePath = true;
            }
            $.layers.emplace_back();
        }

        Priv(size_t logSize, vector<Priv<Layer>> &&layers,
             StringRef tilesPath, size_t tileCount)
        :
        tileLogSize(logSize), tileLogByteSize((logSize << 1) + 2), layers(layers),
        tilesPath(tilesPath), tileCount(tileCount), isUniquePath(false)
        {
            $.tileCache.resize(tileCount);
        }
        
        ~Priv() {
            if ($.isUniquePath) {
                uint32_t removed;
                sys::fs::remove_all($.tilesPath, removed);
            }
        }
        
        void makeTilePath(size_t i, SmallVectorImpl<char> *outPath)
        {
            outPath->clear();
            raw_svector_ostream os(*outPath);
            using namespace sys;
            os << $.tilesPath << '/' << i << ".rgba";
            os.flush();
        }
        
        // fixme: need to be able to unmap stale tiles after threshold reached
        ArrayRef<uint8_t> tile(size_t i, string *outError)
        {
            assert(i >= 1 && i <= $.tileCount);
            MappedFile &file = $.tileCache[i-1];
            if (!file) {
                string error;
                file = $.loadTile(i, outError);
            }
            if (file)
                file.willNeed();
            return file.data;
        }
        
        MappedFile loadTile(size_t index, string *outError)
        {
            assert(index >= 1 && index <= $.tileCount);
            
            SmallString<260> path;
            $.makeTilePath(index, &path);
            
            MappedFile f(path, outError);
            f.sequential();
            return f;
        }
        
        bool saveTile(size_t i, uint8_t const *image, string *outError);
        
        void applyHistory(vector<History> &from, vector<History> &to);
    };
    MEGA_PRIV_DTOR(Canvas)

    template<>
    struct Priv<Layer> {
        Vec parallax;
        Vec origin;
        vector<Layer::tile_t> tiles;
        size_t quadtreeDepth;

        Priv()
        : parallax{1.0, 1.0}, origin{0.0, 0.0}, quadtreeDepth(0)
        {}

        Priv(Vec parallax, Vec origin, size_t quadtreeDepth, vector<Layer::tile_t> &&tiles)
        : parallax(parallax), origin(origin), quadtreeDepth(quadtreeDepth), tiles(tiles)
        {}
        
        Layer::tile_t *segmentCorner(ptrdiff_t quadrantSize,
                                     ptrdiff_t x, ptrdiff_t y);
        
        void reserve(ptrdiff_t x, ptrdiff_t y, ptrdiff_t w, ptrdiff_t h,
                     ptrdiff_t tileSize);
        void setTile(ptrdiff_t x, ptrdiff_t y, size_t tile);
    };
    MEGA_PRIV_DTOR(Layer)

    struct ReplaceOp {
        size_t index;
        Priv<Layer> layer;
    };
    struct InsertOp {
        size_t index;
        Priv<Layer> layer;
    };
    struct EraseOp {
        size_t index;
    };
    struct MoveOp {
        size_t oldIndex;
        size_t newIndex;
    };
    struct SetParallaxOp {
        size_t index;
        Vec parallax;
    };

    struct History {
        string name;
        enum class Tag { Replace, Insert, Erase, Move, SetParallax } tag;
        
        union {
            ReplaceOp replace;
            InsertOp insert;
            EraseOp erase;
            MoveOp move;
            SetParallaxOp setParallax;
        };
        
        History(StringRef name, ReplaceOp &&replace)
        : name(name), tag(Tag::Replace), replace(replace)
        {}
        History(StringRef name, InsertOp &&insert)
        : name(name), tag(Tag::Insert), insert(insert)
        {}
        History(StringRef name, EraseOp &&erase)
        : name(name), tag(Tag::Erase), erase(erase)
        {}
        History(StringRef name, MoveOp &&move)
        : name(name), tag(Tag::Move), move(move)
        {}
        History(StringRef name, SetParallaxOp &&set)
        : name(name), tag(Tag::SetParallax), setParallax(set)
        {}

        History(History &&h)
        : name(std::move(h.name)), tag(h.tag)
        {
            switch (tag) {
                case Tag::Replace:
                    new (&replace) ReplaceOp(std::move(h.replace));
                    break;
                case Tag::Insert:
                    new (&insert) InsertOp(std::move(h.insert));
                    break;
                case Tag::Erase:
                    new (&erase) EraseOp(std::move(h.erase));
                    break;
                case Tag::Move:
                    new (&move) MoveOp(std::move(h.move));
                    break;
                case Tag::SetParallax:
                    new (&setParallax) SetParallaxOp(std::move(h.setParallax));
                    break;
            }
        }
        ~History() {
            switch (tag) {
                case Tag::Replace:
                    replace.~ReplaceOp();
                    break;
                case Tag::Insert:
                    insert.~InsertOp();
                    break;
                case Tag::Erase:
                    erase.~EraseOp();
                    break;
                case Tag::Move:
                    move.~MoveOp();
                    break;
                case Tag::SetParallax:
                    setParallax.~SetParallaxOp();
                    break;
            }
        }
    };

    //
    // Canvas implementation
    //
    Owner<Canvas> Canvas::create(string *outError)
    {
        outError->clear();
        auto r = createOwner<Canvas>(outError);
        if (!outError->empty())
            return {};
        else
            return r;
    }

    namespace {
        template<typename T>
        Optional<T> intFromNode(yaml::Node *node, SmallVectorImpl<char> &scratch)
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

        Optional<Vec> vecFromNode(yaml::Node *node, SmallVectorImpl<char> &scratch)
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
                return Vec{components[0], components[1]};
            }
            return Optional<Vec>();
        }
    }

    Owner<Canvas> Canvas::load(StringRef path, string *outError)
    {
        using namespace std;
        using namespace llvm;
        using namespace sys;
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
                        Optional<size_t> quadtreeDepth;
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
                                _MEGA_LOAD_ERROR_IF(!(quadtreeDepth = intFromNode<size_t>(layerValueNode, scratch)),
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

        return result;

#undef _MEGA_LOAD_ERROR_IF
error:
        assert(!outError->empty());
        return Owner<Canvas>();
    }

    MEGA_PRIV_GETTER(Canvas, tileLogSize, size_t)
    MEGA_PRIV_GETTER(Canvas, layers, PrivArrayRef<Layer>)
    MEGA_PRIV_GETTER(Canvas, tileCount, size_t)

    size_t Canvas::tileSize()
    {
        return 1 << $.tileLogSize;
    }

    size_t Canvas::tileArea()
    {
        return 1 << ($.tileLogSize << 1);
    }

    size_t Canvas::tileByteSize()
    {
        return 1 << $.tileLogByteSize;
    }
    
    bool Canvas::verifyTiles(string *outError)
    {
        using namespace llvm;
        using namespace sys;
        raw_string_ostream errors(*outError);
        SmallString<260> path;
        bool ok = true;
        for (size_t i = 1; i <= $.tileCount; ++i) {
            $.makeTilePath(i, &path);
            bool isRegular;
            llvm::error_code code = fs::is_regular_file(path.str(), isRegular);
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
    
    bool
    Canvas::loadTileInto(size_t index, MutableArrayRef<uint8_t> outBuffer, string *outError)
    {
        assert(outBuffer.size() == $$.tileByteSize());
        if (index == 0) {
            memset(outBuffer.begin(), 0, $$.tileByteSize());
            return true;
        }
        
        ArrayRef<uint8_t> tile = $.tile(index, outError);
        if (!tile.empty()) {
            assert(outBuffer.size() == tile.size());
            memcpy(outBuffer.data(), tile.data(), tile.size());
            return true;
        } else
            return false;
    }
    
    void
    Canvas::wantTile(size_t index)
    {
        if (index == 0)
            return;
        string error;
        MappedFile file = $.loadTile(index, &error);
        file.willNeed();
    }
    
    struct CallbackInfo {
        function<void (bool, const string &)> userCallback;
        uint8_t *ptr;
        size_t size;
    };
    
    static string streamErrorString(CFReadStreamRef stream)
    {
        CFErrorRef error = CFReadStreamCopyError(stream);
        MEGA_FINALLY({ CFRelease(error); });
        CFStringRef desc = CFErrorCopyDescription(error);
        MEGA_FINALLY({ CFRelease(desc); });
        
        unique_ptr<char[]> buf(new char[CFStringGetLength(desc)*4+1]);
        bool ok = CFStringGetCString(desc, buf.get(), CFStringGetLength(desc)*4+1,
                                     kCFStringEncodingUTF8);
        assert(ok);
        
        return string(buf.get());
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
            string error;
            raw_string_ostream errors(error);
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
    Canvas::loadTileIntoAsync(size_t index, MutableArrayRef<uint8_t> outBuffer,
                              function<void (bool, const string &)> callback)
    {
        assert(index >= 1 && index <= $.tileCount);
        assert(outBuffer.size() >= $$.tileByteSize());
        SmallString<260> path;
        $.makeTilePath(index, &path);
        
        CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL,
                                                               reinterpret_cast<UInt8 const*>(path.c_str()),
                                                               path.size(),
                                                               false);
        MEGA_FINALLY({ CFRelease(url); });
        CFReadStreamRef stream = CFReadStreamCreateWithFile(NULL, url);
        
        auto callbackBuf = new CallbackInfo{
            move(callback),
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
    
    void Canvas::wasMoved(StringRef newPath)
    {
        $.tilesPath = newPath;
        $.isUniquePath = false;
    }
    
    // fixme leave empty tiles as 0
    // fixme don't copy unaffected tiles
    void Canvas::blit(StringRef name,
                      const void *source,
                      size_t sourcePitch, size_t sourceW, size_t sourceH,
                      size_t destLayer, ptrdiff_t destX, ptrdiff_t destY,
                      pixel_t (*blendFunc)(pixel_t, pixel_t))
    {
        using namespace std;
        using namespace tbb;
        Priv<Layer> &layer = $.layers[destLayer];
        $.undo.emplace_back(name, ReplaceOp{destLayer, layer});
        layer.reserve(destX, destY, sourceW, sourceH, $$.tileSize());
        Array2DRef<pixel_t> sourcePixels(reinterpret_cast<pixel_t const*>(source),
                                         sourcePitch,
                                         sourceH);
        Vec origin = layer.origin;
        ptrdiff_t tileLogSize = $.tileLogSize;
        ptrdiff_t tileSize = $$.tileSize();
        ptrdiff_t destXO = destX - ptrdiff_t(origin.x);
        ptrdiff_t destYO = destY - ptrdiff_t(origin.y);
        ptrdiff_t loTileX = destXO >> tileLogSize;
        ptrdiff_t loTileY = destYO >> tileLogSize;
        ptrdiff_t hiTileX = (destXO + ptrdiff_t(sourceW) + tileSize - 1) >> tileLogSize;
        ptrdiff_t hiTileY = (destYO + ptrdiff_t(sourceH) + tileSize - 1) >> tileLogSize;
        
        blocked_range2d<ptrdiff_t> range(loTileY, hiTileY, loTileX, hiTileX);
        std::atomic<size_t> nextTile($.tileCount+1);
        
        parallel_for(range, [&](blocked_range2d<ptrdiff_t> const &subrange) {
            unique_ptr<pixel_t[]> outPixelBuf(new pixel_t[$$.tileArea()]);
            MutableArray2DRef<pixel_t> outPixels(outPixelBuf.get(), tileSize, tileSize);
            string error;

            for (ptrdiff_t ytile = subrange.rows().begin(),
                    ytend = subrange.rows().end(),
                    ysrc = ytile*tileSize - destYO;
                 ytile < ytend;
                 ++ytile, ysrc += tileSize)
                for (ptrdiff_t xtile = subrange.cols().begin(),
                        xtend = subrange.cols().end(),
                        xsrc = xtile*tileSize - destXO;
                     xtile < xtend;
                     ++xtile, xsrc += tileSize) {
                    Layer::tile_t tileIndex = Layer(layer).tile(xtile, ytile);

                    if (tileIndex != 0) {
                        ArrayRef<uint8_t> origTile = $.tile(tileIndex, &error);
                        assert(!origTile.empty());
                        Array2DRef<pixel_t> destPixels(reinterpret_cast<pixel_t const*>(origTile.data()),
                                                       tileSize, tileSize);
                        for (ptrdiff_t ypix = 0; ypix < tileSize; ++ypix)
                            for (ptrdiff_t xpix = 0; xpix < tileSize; ++xpix)
                                if (xsrc+xpix >= 0 && xsrc+xpix < sourceW && ysrc+ypix >= 0 && ysrc+ypix < sourceH)
                                    outPixels[ypix][xpix] = blendFunc(sourcePixels[xsrc+xpix][ysrc+ypix],
                                                                      destPixels[xpix][ypix]);
                                else
                                    outPixels[ypix][xpix] = blendFunc({0,0,0,0}, destPixels[xpix][ypix]);
                    } else {
                        for (ptrdiff_t ypix = 0; ypix < tileSize; ++ypix)
                            for (ptrdiff_t xpix = 0; xpix < tileSize; ++xpix)
                                if (xsrc+xpix >= 0 && xsrc+xpix < sourceW && ysrc+ypix >= 0 && ysrc+ypix < sourceH)
                                    outPixels[ypix][xpix] = blendFunc(sourcePixels[xsrc+xpix][ysrc+ypix],
                                                                      {0,0,0,0});
                                else
                                    outPixels[ypix][xpix] = blendFunc({0,0,0,0}, {0,0,0,0});
                    }
                    
                    size_t newTile = nextTile.fetch_add(1);
                    bool ok = $.saveTile(newTile,
                                         reinterpret_cast<uint8_t const*>(outPixelBuf.get()),
                                         &error);
                    assert(ok);
                    layer.setTile(xtile, ytile, newTile);
                }
        });
        
        $.tileCount = nextTile.load() - 1;
        $.tileCache.resize($.tileCount);
    }
    
    void Canvas::insertLayer(llvm::StringRef undoName, size_t index)
    {
        assert(index <= $.layers.size());
        $.layers.emplace($.layers.begin()+index);
        $.undo.emplace_back(undoName, EraseOp{index});
    }
    
    void Canvas::deleteLayer(llvm::StringRef undoName, size_t index)
    {
        assert(index < $.layers.size());
        auto it = $.layers.begin()+index;
        $.undo.emplace_back(undoName, InsertOp{index, move(*it)});
        $.layers.erase(it);
    }
    
    bool Priv<Canvas>::saveTile(size_t index, const uint8_t *image, string *outError)
    {
        assert(index != 0 && index > $.tileCount);
        
        SmallString<260> path;
        $.makeTilePath(index, &path);

        FILE *out;
        do {
            out = fopen(path.c_str(), "wb");
        } while (!out && errno == EINTR);
        if (!out) {
            *outError = strerror(errno);
            return false;
        }
        
        size_t written;
        do {
            written = fwrite(image, $$.tileByteSize(), 1, out);
        } while (written == 0 && errno == EINTR);
        if (written == 0) {
            *outError = strerror(errno);
            return false;
        }
        
        return true;
    }
    
    void Canvas::moveLayer(llvm::StringRef undoName, size_t oldIndex, size_t newIndex)
    {
        $.undo.emplace_back(undoName, MoveOp{oldIndex, newIndex});
        swap($.layers[oldIndex], $.layers[newIndex]);
    }
    
    void Canvas::setLayerParallax(llvm::StringRef undoName, size_t index, Mega::Vec parallax)
    {
        $.undo.emplace_back(undoName, SetParallaxOp{index, $.layers[index].parallax});
        $.layers[index].parallax = parallax;
    }
    
    StringRef
    Canvas::undoName()
    {
        if ($.undo.empty())
            return {};
        else
            return $.undo.back().name;
    }

    StringRef
    Canvas::redoName()
    {
        if ($.redo.empty())
            return {};
        else
            return $.redo.back().name;
    }
    
    void
    Canvas::undo()
    {
        $.applyHistory($.undo, $.redo);
    }
    
    void
    Canvas::redo()
    {
        $.applyHistory($.redo, $.undo);
    }
    
    void
    Priv<Canvas>::applyHistory(vector<History> &from, vector<History> &to)
    {
        assert(!from.empty());
        History item = move(from.back());
        from.pop_back();
        switch (item.tag) {
            case History::Tag::Replace:
                swap(item.replace.layer, $.layers[item.replace.index]);
                to.emplace_back(move(item));
                break;
            case History::Tag::Erase: {
                auto it = $.layers.begin() + item.erase.index;
                to.emplace_back(move(item.name), InsertOp{item.erase.index, move(*it)});
                $.layers.erase(it);
                break;
            }
            case History::Tag::Insert: {
                $.layers.emplace($.layers.begin() + item.insert.index, move(item.insert.layer));
                to.emplace_back(move(item.name), EraseOp{item.insert.index});
                break;
            }
            case History::Tag::Move:
                swap($.layers[item.move.oldIndex], $.layers[item.move.newIndex]);
                to.emplace_back(move(item));
                break;
            case History::Tag::SetParallax:
                swap($.layers[item.setParallax.index].parallax, item.setParallax.parallax);
                to.emplace_back(move(item));
                break;
        }
    }

    //
    // Layer implementation
    //
    MEGA_PRIV_GETTER(Layer, parallax, Vec)
    MEGA_PRIV_GETTER(Layer, origin, Vec)
    
    static bool isPowerOfTwo(size_t x)
    {
        return (x & (x - 1)) == 0 && x != 0;
    }
    
    Layer::SegmentRef
    Layer::segment(size_t segmentSize, ptrdiff_t x, ptrdiff_t y)
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
    
    Layer::tile_t
    Layer::tile(ptrdiff_t x, ptrdiff_t y)
    {
        return $$.segment(1, x, y)[0];
    }
    
    Layer::tile_t *
    Priv<Layer>::segmentCorner(ptrdiff_t quadrantSize,
                               ptrdiff_t x, ptrdiff_t y)
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
    
    void
    Priv<Layer>::reserve(ptrdiff_t x, ptrdiff_t y, ptrdiff_t w, ptrdiff_t h, ptrdiff_t tileSize)
    {
        if ($.quadtreeDepth == 0) {
            $.origin = Vec{double(x + w/2), double(y + h/2)};
            size_t size = tileSize;
            do {
                ++$.quadtreeDepth;
                size <<= 1;
            } while (size < w || size < h);
            $.tiles.clear();
            $.tiles.resize(1 << ($.quadtreeDepth << 1));
            errs() << "initialized layer to depth " << $.quadtreeDepth << "\n";
        } else {
            ptrdiff_t radius = tileSize << ($.quadtreeDepth - 1);
            Vec origin = $.origin, lo = origin - radius, hi = origin + radius;
            if (x >= lo.x && y >= lo.y && x+w <= hi.x && y+h <= hi.y)
                return;
            vector<Layer::tile_t> newTiles;
            if (x < lo.x || y < lo.y) {
                size_t targetSize = max(hi.x - x, hi.y - y);
                size_t size = radius << 1, depth = $.quadtreeDepth;
                do {
                    ++depth;
                    origin -= Vec{double(size >> 1), double(size >> 1)};
                    size <<= 1;
                } while (size < targetSize);
                newTiles.resize(1 << (depth << 1));
                copy($.tiles.begin(), $.tiles.end(), newTiles.end() - $.tiles.size());
                $.tiles = move(newTiles);
                $.quadtreeDepth = depth;
                $.origin = origin;
                radius = size >> 1;
                lo = origin - radius;
            }
            if (x+w > hi.x || y+h > hi.y) {
                size_t targetSize = max(x+w - lo.x, y+h - lo.y);
                size_t size = radius << 1, depth = $.quadtreeDepth;
                do {
                    ++depth;
                    origin += Vec{double(size >> 1), double(size >> 1)};
                    size <<= 1;
                } while (size < targetSize);
                $.tiles.resize(1 << (depth << 1));
                $.quadtreeDepth = depth;
                $.origin = origin;
            }
        }
    }
    
    void
    Priv<Layer>::setTile(ptrdiff_t x, ptrdiff_t y, size_t tile)
    {
        Layer::SegmentRef seg = $$.segment(1, x, y);
        assert(seg.tiles.size() == 1);
        
        //fixme gross, but i don't want SegmentRef to be generally mutable
        const_cast<Layer::tile_t&>(seg.tiles[0]) = tile;
    }
}