#pragma once

#include "build.hpp"
#include <Btk/io.hpp>

#if defined(__linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#else
#error "Unsupported platform"
#endif

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

bool MapFile(const char_t *path, IOAccess access, void **ptr, size_t *size) {
    if (path == nullptr) {
        return false;
    }
    if (ptr == nullptr) {
        return false;
    }
    if (size == nullptr) {
        return false;
    }
    *ptr = nullptr;
    *size = 0;

    int flags = 0;
    if (access == IOAccess::ReadOnly) {
        flags = O_RDONLY;
    }
    else if (access == IOAccess::ReadWrite) {
        flags = O_RDWR;
    }
    else if (access == IOAccess::WriteOnly) {
        flags = O_WRONLY;
    }
    else {
        return false;
    }

    int fd = open(path, flags);
    if (fd == -1) {
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }

    void *ptr_ = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr_ == MAP_FAILED) {
        close(fd);
        return false;
    }

    close(fd);
    *ptr = ptr_;
    *size = st.st_size;
    return true;
}

bool UnmapFile(void *ptr, size_t size) {
    if (ptr == nullptr) {
        return false;
    }
    if (size == 0) {
        return false;
    }
    return munmap(ptr, size) == 0;
}

BTK_NS_END2()