#pragma once

#include <Btk/string.hpp>
#include <Btk/io.hpp>

#include <curl/multi.h>
#include <curl/curl.h>

#include <vector>
#include <map>

BTK_NS_BEGIN

class NetworkAsyncContext {

};

/**
 * @brief Use CURL library to parse the url as stream
 * 
 */
class NetworkStream : public IOStream {
    public:
        NetworkStream();
        NetworkStream(u8string_view url) : url(url) {}
        NetworkStream(const NetworkStream& other) = delete;
        ~NetworkStream();

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
    private:
        CURL *   curl_alloc();
        size_t   curl_write_cb(char *buf, size_t size);
        size_t   curl_header_cb(char *buf, size_t size);

        u8string url;

        // Curl field
        CURL    *curl = nullptr;
        CURLcode curlerr = CURLE_OK;

        // Temp buffer for cache
        std::vector<uint8_t> buffer;
};


inline u8string NetworkStream::error() {
    return curl_easy_strerror(curlerr);
}

// --- CURL ---
inline CURL     *NetworkStream::curl_alloc() {
    auto handle = curl_easy_init();

    curl_write_callback write_cb = [] (char *buf, size_t size, size_t n, void *self) -> size_t {
        return static_cast<NetworkStream*>(self)->curl_write_cb(buf, size * n);
    };
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(this));

    return handle;
}

BTK_NS_END