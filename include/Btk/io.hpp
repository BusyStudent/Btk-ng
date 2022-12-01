#pragma once

#include <Btk/defs.hpp>
#include <cstdio>
#include <cerrno>

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

/**
 * @brief Wrapper for FILE *
 * 
 */
class FileStream final : public IOStream {
    public:
        FileStream();
        ~FileStream();

        bool    open(const char_t *path, const char_t *mode) override;
        bool    close()                             override;
        bool    flush()                             override;
        bool    seek(int64_t offset, int whence)    override;
        int64_t read(void *buf, size_t size)        override;
        int64_t write(const void *buf, size_t size) override;

        // Query the stream info
        bool     seekable() override;
        int64_t  tell()     override;
        u8string error()    override;


        void  attach(FILE *fp) noexcept;
        FILE *detach()         noexcept;
        
    private:
        int   errcode = 0;
        FILE *fp      = nullptr;
};


// Impl for FileStream
inline FileStream::FileStream() {}
inline FileStream::~FileStream() {
    if (fp) {
        ::fclose(fp);
    }
}
inline bool FileStream::close() {
    FILE *fp = detach();
    if (::fclose(fp) != 0) {
        errcode = errno;
        return false;
    }
    return true;
}
inline bool FileStream::flush() {
    int ret = ::fflush(fp);
    errcode = errno;
    return ret == 0;
}
inline bool FileStream::seek(int64_t offset, int whence) {
    int ret = ::fseek(fp, offset, whence);
    errcode = errno;
    return ret == 0;
}
inline bool FileStream::open(const char_t *path, const char_t *mode) {
    FileStream::close();
    fp = ::fopen(path, mode);
    errcode = errno;
    return fp != nullptr;
}
inline void FileStream::attach(FILE *f) noexcept {
    FileStream::close();
    fp = f;
}
inline FILE* FileStream::detach() noexcept {
    FILE *f = fp;
    fp = nullptr;
    return f;
}
inline bool FileStream::seekable() {
    int ret = ::fseek(fp, 0, SEEK_CUR);
    return ret == 0;
}
inline int64_t FileStream::tell() {
    fpos_t ps;
    int ret = ::fgetpos(fp, &ps);
    if (ret == 0) {
        return ps;
    }
    errcode = errno;
    return -1;
}
inline int64_t FileStream::write(const void *buf, size_t size) {
    auto ret = ::fwrite(buf, 1, size, fp);
    errcode = errno;
    return ret;
}
inline int64_t FileStream::read(void *buf, size_t size) {
    auto ret = ::fread(buf, 1, size, fp);
    errcode = errno;
    return ret;
}
inline u8string FileStream::error() {
    return ::strerror(errcode);
}

BTK_NS_END