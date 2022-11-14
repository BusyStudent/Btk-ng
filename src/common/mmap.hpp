#pragma once

#include "build.hpp"
#include <Btk/io.hpp>

#if   defined(__linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#elif defined(_WIN32)
#include <windows.h>
#include <wrl.h>
#undef min
#undef max
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

#if defined(__linux)
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
#elif defined(_WIN32)
    using namespace Microsoft::WRL::Wrappers;
    HandleT<HandleTraits::FileHandleTraits> file;
    HandleT<HandleTraits::FileHandleTraits> view;

    DWORD flags;
    DWORD prot;

    switch (access) {
        case IOAccess::ReadOnly : flags = GENERIC_READ;  prot = PAGE_READONLY; break;
        case IOAccess::WriteOnly : flags = GENERIC_WRITE; prot = PAGE_WRITECOPY; break;
        case IOAccess::ReadWrite : flags = GENERIC_READ | GENERIC_WRITE; prot = PAGE_READWRITE; break;
    }

    file.Attach(
        CreateFileW(
            reinterpret_cast<LPCWSTR>(u8string_view(path).to_utf16().c_str()),
            flags,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        )
    );
    if (!file.IsValid()) {
        return false;
    }
    DWORD size = GetFileSize(file.Get(), nullptr);
    view.Attach(
        CreateFileMapping(
            file.Get(),
            nullptr,
            prot,
            0,
            0,
            0
        )
    );
    if (!view.IsValid()) {
        return false;
    }
#else
    return false;
#endif
}

bool UnmapFile(void *ptr, size_t size) {
    if (ptr == nullptr) {
        return false;
    }
    if (size == 0) {
        return false;
    }
#if defined(__linux)
    return munmap(ptr, size) == 0;
#elif defined(_WIN32)
    return UnmapViewOfFile(ptr);
#else
    return false;
#endif
}

BTK_NS_END2()