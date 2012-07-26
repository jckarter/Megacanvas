//
//  MappedFile.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/24/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_MappedFile_hpp
#define Megacanvas_MappedFile_hpp

#include <cstddef>
#include <string>
#include <utility>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

namespace Mega {
    struct MappedFile {
        llvm::ArrayRef<std::uint8_t> data;
        
        MappedFile() : data() {}
        MappedFile(llvm::StringRef path, std::string *outError) : data() { load(path, outError); }
        ~MappedFile() { reset(); }
        
        MappedFile(const MappedFile&) = delete;
        void operator=(const MappedFile&) = delete;
        
        MappedFile(MappedFile &&x) : data(x.data)
        {
            x.data = llvm::ArrayRef<std::uint8_t>();
        }
        
        MappedFile &operator=(MappedFile &&x)
        {
            std::swap(data, x.data);
            return *this;
        }
        
        MappedFile &operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }
        
        explicit operator bool() const
        {
            return data.begin() != nullptr;
        }
        
        using iterator = std::uint8_t const *;
        using const_iterator = std::uint8_t const *;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using value_type = std::uint8_t;
        using reference = std::uint8_t &;
        using const_reference = std::uint8_t const &;
        
        std::uint8_t const *begin() const { return data.begin(); }
        std::uint8_t const *end() const { return data.end(); }
        std::size_t size() const { return data.size(); }
        
        bool load(llvm::StringRef path, std::string *outError);
        void reset();
        
        void dontNeed() const;
        void willNeed() const;
        void free() const;
    };
}

#endif
