#pragma once

#include <Btk/defs.hpp>

BTK_NS_BEGIN

enum class SeekOrigin : int {
    Begin = SEEK_SET,
    Current = SEEK_CUR,
    End = SEEK_END
};

class IOStream : public Any {
    public:
        virtual bool    open(const char_t *path, const char_t *mode) = 0;
        virtual bool    close() = 0;
        virtual bool    flush() = 0;
        virtual ssize_t read(void *buf, size_t size) = 0;
        virtual ssize_t write(const void *buf, size_t size) = 0;
        virtual bool    seek(ssize_t offset, int whence) = 0;
        virtual ssize_t tell() = 0;
};
class IODevice : public IOStream {
    public:

};

BTK_NS_END