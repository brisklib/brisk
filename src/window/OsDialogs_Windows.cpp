/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#define BRISK_ALLOW_OS_HEADERS 1
#define NOMINMAX 1
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wrl.h>

#include <brisk/window/OsDialogs.hpp>
#include <brisk/window/WindowApplication.hpp>

#include <brisk/core/internal/COMInit.hpp>
#include <brisk/core/Log.hpp>

#include <brisk/window/Window.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/graphics/OsWindowHandle.hpp>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace Brisk {

void Shell::openURLInBrowser(std::string_view url) {
    ShellExecuteW(NULL, L"open", utf8ToWcs(url).c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void Shell::openFolder(const fs::path& path) {
    openURLInBrowser(path.string());
}

void Shell::openFileInDefaultApp(const fs::path& path) {
    openURLInBrowser(path.string());
}

template <typename T>
struct CoTaskMemPtr {
    constexpr CoTaskMemPtr() noexcept : ptr(nullptr) {}

    CoTaskMemPtr(CoTaskMemPtr&&) noexcept      = default;
    CoTaskMemPtr(const CoTaskMemPtr&) noexcept = delete;

    CoTaskMemPtr& operator=(CoTaskMemPtr&& other) noexcept {
        swap(other);
        return *this;
    }

    void swap(CoTaskMemPtr& other) noexcept {
        std::swap(ptr, other.ptr);
    }

    ~CoTaskMemPtr() noexcept {
        if (ptr)
            CoTaskMemFree(ptr);
    }

    T** ReleaseAndGetAddressOf() noexcept {
        Release();
        return &ptr;
    }

    T* Get() const noexcept {
        return ptr;
    }

    void Release() noexcept {
        CoTaskMemPtr{}.swap(*this);
    }

private:
    T* ptr;
};

template <typename T>
static std::optional<T> getFirst(const std::vector<T>& values) {
    if (values.empty())
        return std::nullopt;
    return values.front();
}

inline HWND handleFromWindow(OsWindow* window, HWND fallback = 0) {
    return window ? window->getHandle().hWnd() : fallback;
}

template <std::derived_from<IFileDialog> Dialog>
static std::vector<fs::path> pathDialog(OsWindow* window, std::span<const Shell::FileDialogFilter> filters,
                                        FILEOPENDIALOGOPTIONS flags, const fs::path& defaultPath) {
    COMInitializer com;
    std::vector<fs::path> results;
    if (!com) {
        LOG_ERROR(dialogs, "COM initialization failed");
        return results;
    }
    ComPtr<Dialog> dialog;
    HRESULT result;
    if constexpr (std::derived_from<Dialog, IFileOpenDialog>)
        result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                  IID_PPV_ARGS(dialog.ReleaseAndGetAddressOf()));
    else
        result = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                                  IID_PPV_ARGS(dialog.ReleaseAndGetAddressOf()));
    if (!SUCCEEDED(result)) {
        LOG_ERROR(dialogs, "Cannot create CLSID_FileOpenDialog");
        return results;
    }
    FILEOPENDIALOGOPTIONS opts;
    if (!SUCCEEDED(dialog->GetOptions(&opts)))
        return results;
    if (!SUCCEEDED(dialog->SetOptions(opts | flags)))
        return results;

    if (!filters.empty()) {
        std::wstring buffer;
        std::vector<COMDLG_FILTERSPEC> filt(filters.size());
        for (size_t i = 0; i < filt.size(); i++) {
            BRISK_ASSERT(!filters[i].filters.empty());
            buffer += utf8ToWcs(filters[i].description);
            buffer.push_back(L'\0');
            buffer += utf8ToWcs(join(filters[i].filters, ";"));
            buffer.push_back(L'\0');
        }
        size_t offset = 0;
        for (size_t i = 0; i < filt.size(); i++) {
            filt[i].pszName = buffer.data() + offset;
            offset          = buffer.find(L'\0', offset) + 1;
            filt[i].pszSpec = buffer.data() + offset;
            offset          = buffer.find(L'\0', offset) + 1;
        }
        dialog->SetFileTypes(filt.size(), filt.data());
        if (!filters.front().filters.empty()) {
            std::wstring ext = utf8ToWcs(filters.front().filters.front()).substr(2);
            dialog->SetDefaultExtension(ext.c_str());
        }
    }

    ComPtr<IShellItem> defaultItem;
    if (SUCCEEDED(SHCreateItemFromParsingName(defaultPath.wstring().c_str(), NULL,
                                              IID_PPV_ARGS(defaultItem.ReleaseAndGetAddressOf())))) {
        dialog->SetFolder(defaultItem.Get());
    }

    result = dialog->Show(handleFromWindow(window));
    if (!SUCCEEDED(result)) {
        if (result != HRESULT_FROM_WIN32(ERROR_CANCELLED))
            LOG_ERROR(dialogs, "IFileOpenDialog::Show() failed");
        return results;
    }

    if constexpr (std::derived_from<Dialog, IFileOpenDialog>) {
        ComPtr<IShellItemArray> items;
        if (!SUCCEEDED(dialog->GetResults(items.ReleaseAndGetAddressOf())))
            return results;

        DWORD count = 0;
        if (!SUCCEEDED(items->GetCount(&count))) {
            return results;
        }
        for (DWORD i = 0; i < count; ++i) {
            ComPtr<IShellItem> item;
            if (!SUCCEEDED(items->GetItemAt(i, item.ReleaseAndGetAddressOf()))) {
                return results;
            }
            CoTaskMemPtr<wchar_t> path;
            if (!SUCCEEDED(item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, path.ReleaseAndGetAddressOf())))
                return results;
            results.push_back(path.Get());
        }
    } else {
        ComPtr<IShellItem> item;
        if (!SUCCEEDED(dialog->GetResult(item.ReleaseAndGetAddressOf())))
            return results;
        CoTaskMemPtr<wchar_t> path;
        if (!SUCCEEDED(item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, path.ReleaseAndGetAddressOf())))
            return results;
        results.push_back(path.Get());
    }
    return results;
}

std::optional<fs::path> Shell::showFolderDialog(const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OsWindow* window) {
        result = getFirst(pathDialog<IFileOpenDialog>(window, {}, FOS_PICKFOLDERS, defaultPath));
    });
    return result;
}

std::optional<fs::path> Shell::showOpenDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OsWindow* window) {
        result = getFirst(pathDialog<IFileOpenDialog>(window, filters, FOS_FILEMUSTEXIST, defaultPath));
    });
    return result;
}

std::optional<fs::path> Shell::showSaveDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::optional<fs::path> result;
    windowApplication->systemModal([&](OsWindow* window) {
        result = getFirst(pathDialog<IFileSaveDialog>(window, filters, 0, defaultPath));
    });
    return result;
}

std::vector<fs::path> Shell::showOpenDialogMulti(std::span<const FileDialogFilter> filters,
                                                 const fs::path& defaultPath) {
    std::vector<fs::path> result;
    windowApplication->systemModal([&](OsWindow* window) {
        result = pathDialog<IFileOpenDialog>(window, filters, FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT,
                                             defaultPath);
    });
    return result;
}

static DialogResult showChildDialog(OsWindow* window, DialogButtons buttons, MessageBoxType type,
                                    std::string_view title, std::string_view message) {
    PCWSTR icon = TD_INFORMATION_ICON;
    switch (type) {
    case MessageBoxType::None:
        icon = nullptr;
        break;
    case MessageBoxType::Error:
        icon = TD_ERROR_ICON;
        break;
    case MessageBoxType::Warning:
        icon = TD_WARNING_ICON;
        break;
    case MessageBoxType::Security:
        icon = TD_SHIELD_ICON;
        break;
    default:
        break;
    }
    TASKDIALOG_COMMON_BUTTON_FLAGS btns = 0;
    if (buttons && DialogButtons::OK)
        btns |= TDCBF_OK_BUTTON;
    if (buttons && DialogButtons::Cancel)
        btns |= TDCBF_CANCEL_BUTTON;
    if (buttons && DialogButtons::Yes)
        btns |= TDCBF_YES_BUTTON;
    if (buttons && DialogButtons::No)
        btns |= TDCBF_NO_BUTTON;
    if (buttons && DialogButtons::Close)
        btns |= TDCBF_CLOSE_BUTTON;
    if (buttons && DialogButtons::Retry)
        btns |= TDCBF_RETRY_BUTTON;
    int btn = 0;
    if (!SUCCEEDED(TaskDialog(handleFromWindow(window), GetModuleHandleW(nullptr), utf8ToWcs(title).c_str(),
                              utf8ToWcs(message).c_str(), nullptr, btns, icon, &btn))) {
        LOG_ERROR(dialogs, "TaskDialog() failed");
        return DialogResult::Cancel;
    }
    switch (btn) {
    case IDOK:
        return DialogResult::OK;
    case IDCANCEL:
        return DialogResult::Cancel;
    case IDYES:
        return DialogResult::Yes;
    case IDNO:
        return DialogResult::No;
    case IDCLOSE:
        return DialogResult::Close;
    case IDRETRY:
        return DialogResult::Retry;

    default:
        return DialogResult::Cancel;
    }
}

DialogResult Shell::showDialog(std::string_view title, std::string_view message, DialogButtons buttons,
                               MessageBoxType type) {
    DialogResult result;
    windowApplication->systemModal([&](OsWindow* window) {
        result = showChildDialog(window, buttons, type, title, message);
    });
    return result;
}

} // namespace Brisk
