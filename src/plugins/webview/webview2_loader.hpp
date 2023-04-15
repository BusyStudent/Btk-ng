#pragma once
#include "common/win32/win32.hpp" //< For RegKey
#include "WebView2.h"

BTK_PRIV_BEGIN

// From https://github.com/webview/webview/blob/master/webview.h

auto client_key = L"SOFTWARE\\Microsoft\\EdgeUpdate\\ClientState\\";
auto last_release_guid = L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";

using CreateWebViewEnvironmentWithOptionsInternal_t =
    HRESULT(STDMETHODCALLTYPE *)(
        bool, int, PCWSTR, IUnknown *,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);

inline HRESULT __stdcall Btk_CreateCoreWebView2EnvironmentWithOptions(
    PCWSTR browserExecutableFolder, 
    PCWSTR userDataFolder, 
    ICoreWebView2EnvironmentOptions* environmentOptions, 
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler) 
{
    using Win32::RegKey;

    // Temp unsupport browserExecutableFolder
    BTK_ASSERT(!browserExecutableFolder);

    // Find installed client
    RegKey key(HKEY_LOCAL_MACHINE, client_key, 0, KEY_READ | KEY_WOW64_32KEY);
    if (!key) {
        // Try user
        key = RegKey(HKEY_CURRENT_USER, client_key, 0, KEY_READ | KEY_WOW64_32KEY);
        if (!key) {
            // Bad
            return E_FAIL;
        }
    }

    // Try enum
    std::wstring finded;
    for (auto sub : key) {
        finded = sub;
        if (sub == last_release_guid) {
            // Best id
            break;
        }
    }
    if (finded.empty()) {
        // No client
        return E_FAIL;
    }

    auto subkey = key.subkey(finded.c_str());
    if (!subkey) {
        return E_FAIL;
    }

    auto embed_value = subkey.query_value_string(L"EBWebView");
    if (embed_value.empty()) {
        return E_FAIL;
    }

    // Process this value
    if (embed_value.back() != L'\\' && embed_value.back() != L'/') {
        embed_value += L'\\';
    }

    embed_value += L"EBWebView\\";
#if defined(_M_X64) || defined(__x86_64__)
    embed_value += L"x64";
#elif defined(_M_IX86) || defined(__i386__)
    embed_value += L"x86";
#endif
    embed_value += L"\\EmbeddedBrowserWebView.dll";

    // Try load DLL
    HMODULE dll = ::LoadLibraryW(embed_value.c_str());
    if (!dll) {
        return E_FAIL;
    }
    auto create_fn = (CreateWebViewEnvironmentWithOptionsInternal_t) ::GetProcAddress(dll, "CreateWebViewEnvironmentWithOptionsInternal");
    if (!create_fn) {
        ::FreeLibrary(dll);
        return E_FAIL;
    }
    // 0 on installed
    // 1 on embed
    return create_fn(true, 0, userDataFolder, environmentOptions, environmentCreatedHandler);
}

BTK_PRIV_END