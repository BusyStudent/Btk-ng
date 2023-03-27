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
#undef min
#undef max
#else
#error "Unsupported platform"
#endif

BTK_PRIV_BEGIN

inline bool MapFile(u8string_view url, void **out, size_t *size) {
#if defined(__linux)
    int fd = open(url.copy().c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }
    void *ptr = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (ptr == MAP_FAILED) {
        return false;
    }
    *out = ptr;
    *size = st.st_size;
    return true;
#elif defined(_WIN32)
    HANDLE hfile = CreateFileW((LPCWSTR) url.to_utf16().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hfile == INVALID_HANDLE_VALUE) {
        return false;
    }
    HANDLE hmapping = CreateFileMappingW(hfile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(hfile);
    if (hmapping == nullptr) {
        return false;
    }
    void *ptr = MapViewOfFile(hmapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hmapping);
    if (ptr == nullptr) {
        return false;
    }
    *out = ptr;
    *size = GetFileSize(hmapping, nullptr);
    return true;
#else
    FILE *fp = fopen(url.copy().c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void *ptr = Btk_malloc(file_size);
    if (ptr == nullptr) {
        fclose(fp);
        return false;
    }
    if (fread(ptr, 1, file_size, fp) != file_size) {
        fclose(fp);
        free(ptr);
        return false;
    }
    fclose(fp);
    *out = ptr;
    *size = file_size;
    return true;
#endif

}
inline bool UnmapFile(void *out, size_t size) {
#if defined(__linux)
    return munmap(out, size) == 0;
#elif defined(_WIN32)
    return UnmapViewOfFile(out) != 0;
#else
    Btk_free(out);
#endif

}

BTK_PRIV_END