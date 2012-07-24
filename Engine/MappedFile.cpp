//
//  MappedFile.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/24/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Util/MappedFile.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <llvm/ADT/SmallString.h>

namespace Mega {
    MappedFile::MappedFile(llvm::StringRef path, std::string *outError)
    : data()
    {
        llvm::SmallString<260> paths(path);

        int fd;
        do {
            fd = open(paths.c_str(), O_RDONLY);
        } while (fd == -1 && errno == EINTR);
        if (fd == -1) {
            *outError = strerror(errno);
            return;
        }
        
        struct stat stats;
        int err;
        do {
            err = fstat(fd, &stats);
        } while (err == -1 && errno == EINTR);
        if (err == -1) {
            *outError = strerror(errno);
            goto close_fd;
        }
        
        if (stats.st_size == 0) {
            *outError = "file does not have a known size";
            goto close_fd;
        }
        
        void *mapping;
        do {
            mapping = mmap(nullptr, stats.st_size, PROT_READ, 
                           MAP_FILE | MAP_SHARED, fd, 0);
        } while (mapping == nullptr && errno == EINTR);
        
        data = llvm::makeArrayRef(reinterpret_cast<std::uint8_t const*>(mapping),
                                  stats.st_size);
        
    close_fd:
        close(fd);
    }
    
    MappedFile::~MappedFile()
    {
        if (data.begin()) {
            int err;
            do {
                err = munmap(const_cast<std::uint8_t*>(data.begin()),
                             data.size());
            } while (err == -1 && errno == EINTR);
            assert(err != -1);
        }
    }
    
    static void advise(llvm::ArrayRef<std::uint8_t> data, int advice)
    {
        int err;
        do {
            err = madvise(const_cast<std::uint8_t*>(data.begin()), 
                          data.size(), advice);
        } while (err == -1 && errno == EINTR);
    }
    
    void MappedFile::willNeed() const
    {
        advise(data, MADV_WILLNEED);
    }
    void MappedFile::dontNeed() const
    {
        advise(data, MADV_DONTNEED);
    }
    void MappedFile::free() const
    {
        advise(data, MADV_FREE);
    }
}
