#pragma once

#include <Btk/string.hpp>
#include <Btk/io.hpp>

#include <vector>
#include <map>

#if !defined(BTK_USE_CURL) && __has_include(<winhttp.h>)
#define BTK_USE_WINHTTP
#include <windows.h>
#include <winhttp.h>

#undef min
#undef maxs

#pragma comment(lib, "winhttp.lib")
#else
#define BTK_USE_CURL
#include <curl/multi.h>
#include <curl/curl.h>
#endif


BTK_NS_BEGIN

/**
 * @brief Http Method like GET or POST
 * 
 */
enum class HttpMethod : uint8_t {
    Get    = 0,
    Put    = 1,
    Post   = 2,
    Delete = 3
};
/**
 * @brief Http Headers
 * 
 */
class HttpHeader : public std::map<u8string, u8string> { 
    public:
        using std::map<u8string, u8string>::map;
};
/**
 * @brief Class for process url string
 * 
 */
class NetworkUrl {
    public:
        NetworkUrl() = default;
        NetworkUrl(const NetworkUrl &) = default;

        u8string hostname() const;
        u8string schemes()  const;
        u8string path()     const;
        uint32_t port()     const;
    private:
        u8string us;
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

        // Interface methods
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

        void     set_header(u8string_view key, u8string_view value);
        void     set_header(const HttpHeader &header);

        void     set_url(u8string_view url);

        // Send
        bool     send();

        // Http Query
        int      status_code();
        bool     ok();
        HttpHeader response_header();
    private:
        HttpHeader recv_header; //< Response header
        HttpHeader req_header; //< Request header
        u8string url;

#if     defined(BTK_USE_CURL)
        CURL *   curl_alloc();
        size_t   curl_write_cb(char *buf, size_t size);
        size_t   curl_header_cb(char *buf, size_t size);

        // Curl field
        CURL    *curl = nullptr;
        CURLcode curlerr = CURLE_OK;

        // Temp buffer for cache
        std::vector<uint8_t> buffer;
#endif

#if     defined(BTK_USE_WINHTTP)
        static inline HINTERNET WHttpSession = nullptr;

        URL_COMPONENTS urlcomp = {}; //< Buffer to split 
        HINTERNET connection = nullptr;
        HINTERNET request = nullptr;

        bool      status_err = false; //< Received error ?
        bool      received = false; //< Received response ?
        bool      seeking = false; //< Does on seeking

        int64_t   bodylen = -1; //< Body length (-1 for not avliable)
        int64_t   pos     = 0;  //< Position

        int64_t   range_start = -1;
        int64_t   range_end = -1; 
        
        void     whttp_init();
        void     whttp_shutdown();

        // Make a connection to the server
        bool     whttp_connect();
        //  Received the response
        bool     whttp_receive();
        // Make a request to the server & send the request
        bool     whttp_open(const HttpHeader &header);

        // Win Http Utils
        static 
        HttpHeader whttp_query_header(HINTERNET request);
        static
        void       whttp_add_header(HINTERNET request, const HttpHeader &h);
#endif
};

#if     defined(BTK_USE_CURL)
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
#endif

// ---- WinHttp ---
#if       defined(BTK_USE_WINHTTP)
inline NetworkStream::NetworkStream() {
    whttp_init();
}
inline NetworkStream::~NetworkStream() {
    whttp_shutdown();
}
inline bool NetworkStream::flush() {
    return false; //< Unsupported
}
inline bool NetworkStream::open(const char_t *path, const char_t *) {
    set_url(path);
    return send();
}
inline bool NetworkStream::close() {
    if (request) {
        WinHttpCloseHandle(request);
        request = nullptr;
    }
    if (connection) {
        WinHttpCloseHandle(connection);
        connection = nullptr;
    }
    status_err = false;
    received = false;
    urlcomp = {};
    return true;
}
inline bool NetworkStream::send() {
    if (url.empty()) {
        return false;
    }
    auto wstr = url.to_utf16();
    
    // Make components, and set we need parts
    urlcomp = {};
    urlcomp.dwStructSize = sizeof(URL_COMPONENTS);
    urlcomp.dwHostNameLength = DWORD(-1);
    urlcomp.dwUrlPathLength = DWORD(-1);

    BOOL ret;

    ret = WinHttpCrackUrl(
        reinterpret_cast<LPWSTR>(wstr.data()),
        wstr.length(),
        0,
        &urlcomp
    );
    if (!ret) {
        // Bad url
        return false;
    }

    // Connect to server
    if (!whttp_connect())  {
        return false;
    }

    // Make & Send the request
    return whttp_open(req_header);
}
inline bool NetworkStream::seekable() {
    // Check response header status
    auto iter = recv_header.find("Accept-Ranges");
    if (iter != recv_header.end()) {
        return iter->second.contains("bytes");
    }
    return false;
}
inline bool NetworkStream::seek(int64_t offset, int whence) {
    // TODO : Seeking
    if (!seekable()) {
        return false;
    }
    return false;
}
inline int64_t NetworkStream::read(void *buf, size_t size) {
    if (!received) {
        if (!whttp_receive()) {
            return -1;
        }
    }
    DWORD avaliable;
    if (!WinHttpQueryDataAvailable(request, &avaliable)) {
        return -1;
    }
    if (avaliable == 0) {
        return 0; //< No data available
    }
    // Begin read data
    DWORD read_bytes = size;
    DWORD readed = 0;

    if (!WinHttpReadData(request, buf, read_bytes, &readed)) {
        // Error
        status_err = true;
        return -1;
    }
    pos += readed;
    return readed;
}
inline int64_t NetworkStream::write(const void *buf, size_t size) {
    return -1;
}
inline int64_t NetworkStream::tell() {
    return pos; //< Unsupported now
}
inline u8string NetworkStream::error() {
    return {};
}

// ---- Set Parts ---- 
inline void NetworkStream::set_url(u8string_view u) {
    url = u;
}
inline void NetworkStream::set_header(u8string_view key, u8string_view value) {
    req_header[u8string(key)] = value;
}
inline void NetworkStream::set_header(const HttpHeader &header) {
    req_header = header;
}

// ---- Http Query ---
inline int  NetworkStream::status_code() {
    if (!received) {
        if (!whttp_receive()) {
            return -1;
        }
    }

    DWORD code = 0;
    DWORD size = sizeof(code);

    BOOL ret = WinHttpQueryHeaders(
        request, 
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
        WINHTTP_HEADER_NAME_BY_INDEX, 
        &code, 
        &size, 
        WINHTTP_NO_HEADER_INDEX
    );
    if (!ret) {
        return -1;
    }
    return code;
}
inline bool NetworkStream::ok() {
    return status_code() == 200;
}
inline auto NetworkStream::response_header() -> HttpHeader {
    if (!received) {
        if (!whttp_receive()) {
            return {};
        }
    }
    return recv_header;
}

// ---- WinHttp Private ----
inline void NetworkStream::whttp_init() {
    WHttpSession = WinHttpOpen(
        nullptr,
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
    if (WHttpSession == nullptr) {
        // ?
    }
}
inline void NetworkStream::whttp_shutdown() {
    WinHttpCloseHandle(WHttpSession);
}
inline bool NetworkStream::whttp_connect() {
    connection = WinHttpConnect(
        WHttpSession,
        urlcomp.lpszHostName,
        urlcomp.nPort,
        0
    );
    return connection != nullptr;
}
inline bool NetworkStream::whttp_open(const HttpHeader &header) {
    // Locate reference, emm we need a case insective soultion
    std::u16string refer_url;
    auto refer = header.find("Referer");
    if (refer == header.end()) {
        refer = header.find("referer");
    }
    if (refer != header.end()) {
        // Got it
        refer_url = refer->second.to_utf16();
    }
    
    request = WinHttpOpenRequest(
        connection,
        L"GET",
        urlcomp.lpszUrlPath,
        nullptr,
        refer_url.empty() ? WINHTTP_NO_REFERER : reinterpret_cast<LPCWSTR>(refer_url.c_str()),
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0 //< WINHTTP_FLAG_SECURE will cause 12175 Error Code, So why ?
    );

    if (!request) {
        return false;
    }

    whttp_add_header(request, header);

    // Send it 
    bool ret = WinHttpSendRequest(
        request,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0
    );
    if (!ret) {
        // DWORD errcode = GetLastError();
        // printf("GetLastError %d", errcode);
    }
    return ret;
}
inline bool NetworkStream::whttp_receive() {
    received = true;
    if (!WinHttpReceiveResponse(request, nullptr)) {
        status_err = true;
        return false;
    }
    recv_header = whttp_query_header(request);

    // Parse the response header
    auto iter = recv_header.find("Content-Length");
    if (iter != req_header.end()) {
        // Got 
        int bytes;
        if (sscanf(iter->second.c_str(), "%d", &bytes) == 1) {
            bodylen = bytes;
        }
    }

    return true;
}

// --- WinHttp Utils ----
inline auto NetworkStream::whttp_query_header(HINTERNET request) -> HttpHeader {
    HttpHeader header;
    DWORD header_size;
    DWORD code = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX,
        nullptr,
        &header_size,
        WINHTTP_NO_HEADER_INDEX
    );
    std::wstring wstr;
    wstr.resize(header_size / sizeof(wchar_t));

    // Read it
    code = WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX,
        wstr.data(),
        &header_size,
        WINHTTP_NO_HEADER_INDEX
    );
    if (FAILED(code)) {
        return header;
    }

    // Convert to utf8 string
    auto us = u8string::from(wstr);
    wstr.clear();

    for (auto part : us.split_ref("\r\n")) {
        if (!part.contains(":")) {
            // First line
            continue;
        }
        auto line_part = part.split_ref(":");
        header.insert(
            std::make_pair(
                u8string(line_part.at(0)),
                u8string(line_part.at(1))
            )
        );
    }
    return header;
}
inline auto NetworkStream::whttp_add_header(HINTERNET request, const HttpHeader &header) -> void {
    for (auto &[key, value] : header) {
        auto wkey = key.to_utf16();
        auto wvalue = key.to_utf16();

        WINHTTP_EXTENDED_HEADER wheader;
        wheader.pwszName = reinterpret_cast<LPCWSTR>(wkey.c_str());
        wheader.pwszValue = reinterpret_cast<LPCWSTR>(wvalue.c_str());

        WinHttpAddRequestHeadersEx(
            request,
            WINHTTP_ADDREQ_FLAG_ADD,
            WINHTTP_EXTENDED_HEADER_FLAG_UNICODE,
            0,
            1,
            &wheader
        );
    }
}


#endif

inline std::ostream& operator<<(std::ostream& os, const HttpHeader& header) {
    for (auto &[key, value] : header) {
        os << key << ": " << value << "\n";
    }
    return os;
}

BTK_NS_END