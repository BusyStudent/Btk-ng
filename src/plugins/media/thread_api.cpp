#include "build.hpp"
#include "media_priv.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

void SetThreadDescription(const char *name) {

#if defined(_WIN32)
    // Win32 Platform
    static HRESULT (__stdcall *SetThreadDescription)(HANDLE, LPCWSTR) = nullptr;
    if (SetThreadDescription == nullptr) {
        // Try Get it
        HMODULE kernel = ::GetModuleHandleA("Kernel32.dll");
        SetThreadDescription = (HRESULT (__stdcall *)(HANDLE, LPCWSTR)) ::GetProcAddress(kernel, "SetThreadDescription");
    }
    if (SetThreadDescription) {
        auto wname = u8string_view(name).to_utf16();
        if (SUCCEEDED(SetThreadDescription(::GetCurrentThread(), (LPCWSTR) wname.c_str()))) {
            return;
        }
    }

#if defined(_MSC_VER)
    const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    // If is debug
    if (::IsDebuggerPresent()) {
        // Try
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = ::GetCurrentThreadId();
        info.dwFlags = 0;

        ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
#endif
#else

#endif

}

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)
