#pragma once

#include <Btk/defs.hpp>

BTK_NS_BEGIN

enum class SeekOrigin : int {
    Begin   = SEEK_SET,
    Current = SEEK_CUR,
    End     = SEEK_END
};
enum class IOAccess : int {
    ReadOnly  = 1 << 0,
    WriteOnly = 1 << 1,
    ReadWrite = ReadOnly | WriteOnly,
};

BTK_FLAGS_OPERATOR(IOAccess, int);

class IOStream : public Any {
    public:
        virtual bool    open(const char_t *path, const char_t *mode) = 0;
        virtual bool    close() = 0;
        virtual bool    flush() = 0;
        virtual int64_t read(void *buf, size_t size) = 0;
        virtual int64_t write(const void *buf, size_t size) = 0;
        virtual bool    seek(int64_t offset, int whence) = 0;

        // Query the stream info
        virtual bool     seekable() = 0;
        virtual int64_t  tell() = 0;
        virtual u8string error() = 0;
};
class IODevice : public IOStream {
    public:
        
};

BTK_NS_END