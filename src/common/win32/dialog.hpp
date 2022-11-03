#pragma once

#include "build.hpp"
#include <Btk/context.hpp>

#include <ShlObj.h>
#include <wrl.h>
#undef min
#undef max

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;
using Microsoft::WRL::ComPtr;

// Provide win32 platform native dialog
class Win32FileDialog : public AbstractFileDialog {
    public:
        int        run()    override;
        bool       initialize(bool save) override;
        void       set_dir(u8string_view dir) override;
        void       set_title(u8string_view title) override;
        void       set_allow_multi(bool v) override;
        StringList result() override;
    private:
        ComPtr<IFileDialog> dialog;
};

// Win32FileDialog
int  Win32FileDialog::run() {
    // Init etc...
    if (FAILED(dialog->Show(nullptr))) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
bool Win32FileDialog::initialize(bool is_open) {
    const IID *clsid;
    const IID *iid;

    if (is_open) {
        clsid = &CLSID_FileOpenDialog;
        iid   = &__uuidof(IFileOpenDialog);
    }
    else {
        clsid = &CLSID_FileSaveDialog;
        iid   = &__uuidof(IFileSaveDialog);
    }


    HRESULT hr = CoCreateInstance(
        *clsid,
        nullptr,
        CLSCTX_INPROC_SERVER,
        *iid,
        reinterpret_cast<void**>(dialog.ReleaseAndGetAddressOf())
    );
    return SUCCEEDED(hr);
}
void Win32FileDialog::set_dir(u8string_view view) {
    // SHCreateShellItem()
    // dialog->SetFolder();
}
void Win32FileDialog::set_title(u8string_view title) {
    dialog->SetTitle(
        reinterpret_cast<const WCHAR*>(title.to_utf16().c_str())
    );
}
void Win32FileDialog::set_allow_multi(bool v) {    
    FILEOPENDIALOGOPTIONS opt;
    HRESULT hr;
    dialog->GetOptions(&opt);
    dialog->SetOptions(
        v ? opt | FOS_ALLOWMULTISELECT : opt ^ FOS_ALLOWMULTISELECT
    );
}
auto Win32FileDialog::result() -> StringList {
    StringList ret;
    auto add_item = [&](IShellItem *item) {
        WCHAR *str;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &str))) {
            abort();
        }
        ret.emplace_back(u8string::from(str));
        CoTaskMemFree(str);
    };

    // Cast to open dialog
    ComPtr<IFileOpenDialog> odialog;
    ComPtr<IShellItemArray> array;
    ComPtr<IShellItem>      item;
    DWORD                   nitem;
    if (dialog.As(&odialog)) {
        odialog->GetResults(array.GetAddressOf());
        array->GetCount(&nitem);

        for (DWORD i = 0; i < nitem; i++)  {
            array->GetItemAt(i, item.ReleaseAndGetAddressOf());
            add_item(item.Get());
        }
    }
    else {
        dialog->GetResult(&item);
        add_item(item.Get());
    }
    return ret;
}

BTK_NS_END2()