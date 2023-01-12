#include <Btk/plugins/network.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

int main() {
    // Currently unimplemented on curl
#if defined(BTK_USE_WINHTTP)
    NetworkStream stream;
    stream.set_url("http://www.baidu.com");

    if (!stream.send()) {
        std::cout << "Failed to send request" << std::endl;
        return 1;
    }

    std::cout << stream.status_code() << std::endl;
    std::cout << stream.response_header() << std::endl;
    

    u8string content;
    do {
        char buffer[1024] = {0};
        int64_t res = stream.read(buffer, sizeof(buffer));
        if (res <= 0) {
            printf("Status code %d\n", int(res));
            break;
        }
        content.append(buffer, res);
    }
    while (true);
    
    std::cout << content << std::endl;
#endif
}