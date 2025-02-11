/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
#include <brisk/window/Clipboard.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Utilities.hpp>

#include "Windows.h"

namespace Brisk {

ClipboardFormat textFormat = CF_UNICODETEXT;

static bool setClipboardData(ClipboardFormat format, const Bytes& bytes) {
    HGLOBAL mem     = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    uint8_t* locked = (uint8_t*)GlobalLock(mem);
    memcpy(locked, bytes.data(), bytes.size());
    GlobalUnlock(mem);
    if (SetClipboardData(format, mem) == NULL)
        return false;
    return true;
}

static optional<Bytes> getClipboardData(ClipboardFormat format) {
    HGLOBAL mem = GetClipboardData(format);
    if (mem == NULL)
        return nullopt;
    uint8_t* locked = (uint8_t*)GlobalLock(mem);
    if (!locked)
        return nullopt;
    SCOPE_EXIT {
        GlobalUnlock(mem);
    };
    size_t sz = GlobalSize(mem);
    Bytes result(sz);
    memcpy(result.data(), locked, sz);
    return result;
}

static Bytes toNulTerminatedWString(std::string_view text) {
    std::wstring content = utf8ToWcs(text);
    return toBytes(std::span{ content.data(), content.data() + content.size() + 1 });
}

static std::string fromNulTerminatedWString(BytesView text) {
    if (text.size_bytes() < 2)
        return {};
    std::wstring content(text.size_bytes() / 2, ' ');
    memcpy(content.data(), text.data(), content.size() * 2);
    if (content.back() == 0)
        content.resize(content.size() - 1);
    return wcsToUtf8(content);
}

bool setClipboardContent(const ClipboardContent& content) {
    if (!OpenClipboard(NULL))
        return false;
    SCOPE_EXIT {
        CloseClipboard();
    };
    if (!EmptyClipboard())
        return false;

    if (content.text)
        if (!setClipboardData(textFormat, toNulTerminatedWString(*content.text)))
            return false;
    for (const auto& f : content.formats) {
        if (!setClipboardData(f.first, f.second))
            return false;
    }
    return true;
}

ClipboardContent getClipboardContent(std::initializer_list<ClipboardFormat> formats) {
    ClipboardContent result;
    if (!OpenClipboard(NULL))
        return result;
    SCOPE_EXIT {
        CloseClipboard();
    };
    for (ClipboardFormat fmt : formats) {
        auto data = getClipboardData(fmt);
        if (!data)
            continue;
        if (fmt == textFormat) {
            result.text = fromNulTerminatedWString(*data);
        } else {
            result.formats.insert_or_assign(fmt, std::move(*data));
        }
    }
    return result;
}

bool clipboardHasFormat(ClipboardFormat format) {
    return IsClipboardFormatAvailable(format);
}

ClipboardFormat registerClipboardFormat(std::string_view formatID) {
    return RegisterClipboardFormatW(utf8ToWcs(formatID).c_str());
}

} // namespace Brisk
