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
#include <brisk/core/Json.hpp>
#include <fmt/format.h>
#include <cmath>

#define RAPIDJSON_WRITE_DEFAULT_FLAGS rapidjson::kWriteNanAndInfFlag
#define RAPIDJSON_PARSE_DEFAULT_FLAGS                                                                        \
    rapidjson::kParseNanAndInfFlag | rapidjson::kParseFullPrecisionFlag | rapidjson::kParseCommentsFlag |    \
        rapidjson::kParseTrailingCommasFlag

#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/reader.h>

#include <msgpack/pack.hpp>
#include <msgpack/parse.hpp>
#include <msgpack/unpack.hpp>

namespace Brisk {

bool IteratePath::Iterator::operator==(const Iterator& it) const {
    return cur == it.cur && next == it.next;
}

bool IteratePath::Iterator::operator!=(const Iterator& it) const {
    return !(*this == it);
}

std::string_view IteratePath::Iterator::operator*() const {
    return std::string_view(path.data() + cur, next - cur);
}

IteratePath::Iterator& IteratePath::Iterator::operator++() {
    cur  = std::min(next + 1, path.size());
    next = find_next(path, cur);
    return *this;
}

IteratePath::Iterator IteratePath::Iterator::operator++(int) {
    Iterator temp = *this;
    ++*this;
    return temp;
}

IteratePath::Iterator IteratePath::begin() const {
    size_t next = find_next(path, 0);
    return Iterator{ path, 0, next };
}

size_t IteratePath::find_next(std::string_view path, size_t offset) {
    if (offset >= path.size()) {
        return path.size();
    }
    size_t next = path.find_first_of('/', offset);
    if (next == std::string_view::npos) {
        return path.size();
    }
    return next;
}

IteratePath::Iterator IteratePath::end() const {
    return { path, path.size(), path.size() };
}

IteratePath iteratePath(std::string_view s) {
    return IteratePath{ s };
}

namespace {

using namespace rapidjson;

template <typename Writer>
void writeJson(Writer& w, const Json& b) {
    switch (b.type()) {
    case JsonType::Array: {
        const auto& arr = b.access<JsonArray>();
        w.StartArray();
        for (const auto& item : arr) {
            writeJson(w, item);
        }
        w.EndArray();
        return;
    }
    case JsonType::Object: {
        const auto& obj = b.access<JsonObject>();
        w.StartObject();
        for (const auto& item : obj) {
            w.Key(item.first.data(), item.first.size());
            writeJson(w, item.second);
        }
        w.EndObject();
        return;
    }
    case JsonType::String: {
        const auto& str = b.access<JsonString>();
        w.String(str.data(), str.size());
        return;
    }
    case JsonType::SignedInteger: {
        auto sint = b.access<JsonSignedInteger>();
        w.Int64(sint);
        return;
    }
    case JsonType::UnsignedInteger: {
        auto uint = b.access<JsonUnsignedInteger>();
        w.Uint64(uint);
        return;
    }
    case JsonType::Float: {
        auto flt = b.access<JsonFloat>();
        w.Double(flt);
        return;
    }
    case JsonType::Bool: {
        auto boo = b.access<JsonBool>();
        w.Bool(boo);
        return;
    }
    case JsonType::Null: {
        w.Null();
        return;
    }
    default: {
        w.Null();
        return;
    }
    }
}

struct Visitor : public BaseReaderHandler<UTF8<>, Visitor> {
    bool Null() {
        back() = nullptr;
        CommitItem();
        return true;
    }

    bool Bool(bool b) {
        back() = b;
        CommitItem();
        return true;
    }

    bool Int(int i) {
        back() = i;
        CommitItem();
        return true;
    }

    bool Uint(unsigned u) {
        back() = u;
        CommitItem();
        return true;
    }

    bool Int64(int64_t i) {
        back() = i;
        CommitItem();
        return true;
    }

    bool Uint64(uint64_t u) {
        back() = u;
        CommitItem();
        return true;
    }

    bool Double(double d) {
        back() = d;
        CommitItem();
        return true;
    }

    bool String(const char* str, SizeType length, bool copy) {
        back() = std::string(str, length);
        CommitItem();
        return true;
    }

    bool StartObject() {
        back() = JsonObject{};
        jsons.push_back(nullptr);
        keys.push_back(std::move(key));
        return true;
    }

    bool Key(const char* str, SizeType length, bool copy) {
        key = std::string(str, length);
        return true;
    }

    bool EndObject(SizeType memberCount) {
        RAPIDJSON_ASSERT(jsons.size() > 1);
        jsons.pop_back();
        key = std::move(keys.back());
        keys.pop_back();
        CommitItem();
        return true;
    }

    bool StartArray() {
        back() = JsonArray{};
        jsons.push_back(nullptr);
        return true;
    }

    bool EndArray(SizeType elementCount) {
        RAPIDJSON_ASSERT(jsons.size() > 1);
        jsons.pop_back();
        CommitItem();
        return true;
    }

    // MSGPACK...

    bool visit_nil() {
        back() = nullptr;
        CommitItem();
        return true;
    }

    bool visit_boolean(bool b) {
        back() = b;
        CommitItem();
        return true;
    }

    bool visit_positive_integer(uint64_t u) {
        back() = u;
        CommitItem();
        return true;
    }

    bool visit_negative_integer(int64_t i) {
        back() = i;
        CommitItem();
        return true;
    }

    bool visit_float32(float f) {
        back() = f;
        CommitItem();
        return true;
    }

    bool visit_float64(double d) {
        back() = d;
        CommitItem();
        return true;
    }

    bool visit_str(const char* str, uint32_t length) {
        if (keyMode) {
            key = std::string(str, length);
            return true;
        }
        back() = std::string(str, length);
        CommitItem();
        return true;
    }

    bool visit_bin(const char* bin, uint32_t length) {
        back() = std::vector<uint8_t>(bin, bin + length);
        CommitItem();
        return true;
    }

    bool visit_ext(const char* /*v*/, uint32_t /*size*/) {
        back() = nullptr;
        CommitItem();
        return true;
    }

    bool start_array(uint32_t /*num_elements*/) {
        back() = JsonArray{};
        jsons.push_back(nullptr);
        return true;
    }

    bool start_array_item() {
        return true;
    }

    bool end_array_item() {
        return true;
    }

    bool end_array() {
        RAPIDJSON_ASSERT(jsons.size() > 1);
        jsons.pop_back();
        CommitItem();
        return true;
    }

    bool start_map(uint32_t /*num_kv_pairs*/) {
        back() = JsonObject{};
        jsons.push_back(nullptr);
        keys.push_back(std::move(key));
        return true;
    }

    bool start_map_key() {
        keyMode = true;
        return true;
    }

    bool end_map_key() {
        keyMode = false;
        return true;
    }

    bool start_map_value() {
        return true;
    }

    bool end_map_value() {
        return true;
    }

    bool end_map() {
        RAPIDJSON_ASSERT(jsons.size() > 1);
        jsons.pop_back();
        key = std::move(keys.back());
        keys.pop_back();
        CommitItem();
        return true;
    }

    void parse_error(size_t /*parsed_offset*/, size_t /*error_offset*/) {}

    void insufficient_bytes(size_t /*parsed_offset*/, size_t /*error_offset*/) {}

    bool referenced() const {
        return false;
    }

    void set_referenced(bool /*referenced*/) {}

    Json& back() {
        return jsons.back();
    }

    void CommitItem() {
        if (jsons.size() > 1) {
            Json item;
            std::swap(item, back());
            if (auto arr = jsons[jsons.size() - 2].to<JsonArray>()) {
                arr->push_back(std::move(item));
            } else if (auto obj = jsons[jsons.size() - 2].to<JsonObject>()) {
                obj->insert_or_assign(key, std::move(item));
            } else {
                RAPIDJSON_ASSERT(false);
            }
        }
    }

    std::vector<Json> jsons;
    std::string key;
    std::vector<std::string> keys;
    bool keyMode = false;
};

namespace {

struct ByteStream {
    Bytes data;

    void write(const char* buf, size_t len) {
        data.insert(data.end(), (const std::byte*)buf, (const std::byte*)buf + len);
    }
};
} // namespace

void writeMsgpack(msgpack::packer<ByteStream>& w, const Json& b) {
    switch (b.type()) {
    case JsonType::Array: {
        const auto& arr = b.access<JsonArray>();
        w.pack_array(arr.size());
        for (const auto& item : arr) {
            writeMsgpack(w, item);
        }
        return;
    }
    case JsonType::Object: {
        const auto& obj = b.access<JsonObject>();
        w.pack_map(obj.size());
        for (const auto& item : obj) {
            w.pack_str(item.first.size()).pack_str_body(item.first.data(), item.first.size());
            writeMsgpack(w, item.second);
        }
        return;
    }
    case JsonType::String: {
        const auto& str = b.access<JsonString>();
        w.pack_str(str.size()).pack_str_body(str.data(), str.size());
        return;
    }
    case JsonType::SignedInteger: {
        auto sint = b.access<JsonSignedInteger>();
        w.pack_int64(sint);
        return;
    }
    case JsonType::UnsignedInteger: {
        auto uint = b.access<JsonUnsignedInteger>();
        w.pack_uint64(uint);
        return;
    }
    case JsonType::Float: {
        auto flt = b.access<JsonFloat>();
        w.pack_double(flt);
        return;
    }
    case JsonType::Bool: {
        auto boo = b.access<JsonBool>();
        boo ? w.pack_true() : w.pack_false();
        return;
    }
    case JsonType::Null: {
        w.pack_nil();
        return;
    }
    default: {
        w.pack_nil();
        return;
    }
    }
}

} // namespace

std::string Json::toJson(int indent) const {
    rapidjson::StringBuffer s;
    if (indent == 0) {
        rapidjson::Writer w(s);
        writeJson(w, *this);
    } else {
        rapidjson::PrettyWriter w(s);
        if (indent < 0)
            w.SetIndent('\t', -indent);
        else
            w.SetIndent(' ', indent);
        writeJson(w, *this);
    }
    return std::string(s.GetString(), s.GetSize());
}

std::optional<Json> Json::fromJson(const std::string& s) {
    rapidjson::StringStream ss(s.c_str());
    rapidjson::Reader r;
    Visitor visitor;
    visitor.jsons.push_back(nullptr);
    auto e = r.Parse(ss, visitor);
    if (e.IsError())
        return std::nullopt;
    RAPIDJSON_ASSERT(visitor.jsons.size() == 1);
    return visitor.back();
}

Bytes Json::toMsgPack() const {
    ByteStream bs;
    msgpack::packer<ByteStream> pack(bs);

    writeMsgpack(pack, *this);

    return std::move(bs.data);
}

std::optional<Json> Json::fromMsgPack(const BytesView& s) {
    Visitor visitor;
    visitor.jsons.push_back(nullptr);
    msgpack::detail::parse_helper<Visitor> r(visitor);
    size_t offs = 0;
    if (r.execute((const char*)s.data(), s.size(), offs) != msgpack::PARSE_SUCCESS)
        return std::nullopt;
    RAPIDJSON_ASSERT(visitor.jsons.size() == 1);
    return visitor.back();
}

namespace {

constexpr double int64MinAsDouble  = -9223372036854775808.0; /* -2^63, exactly representable */
constexpr double int64MaxAsDouble  = 9223372036854775808.0;  /* 2^63 */
constexpr double uint64MaxAsDouble = 18446744073709551616.0; /* 2^64 */

/* Compares a Float value with an integer Json value by exact value.
 * A float with a fractional part (or non-finite) never equals an integer.
 * Out-of-range floats never equal an integer either. */
bool floatEqualsInt(double f, const Json& i) {
    if (std::isinf(f) || std::isnan(f) || f != std::floor(f))
        return false; // fractional part, or not a finite number
    if (i.is<JsonSignedInteger>()) {
        if (f < int64MinAsDouble || f >= int64MaxAsDouble)
            return false;
        return static_cast<JsonSignedInteger>(f) == i.access<JsonSignedInteger>();
    } else {
        if (f < 0 || f >= uint64MaxAsDouble)
            return false;
        return static_cast<JsonUnsignedInteger>(f) == i.access<JsonUnsignedInteger>();
    }
}

/* Compares a SignedInteger with an UnsignedInteger by exact value.
 * Negative signed values never equal unsigned values, and positive int64
 * values are always exactly representable as uint64. */
bool signedEqualsUnsigned(const Json& s, const Json& u) {
    JsonSignedInteger si = s.access<JsonSignedInteger>();
    if (si < 0)
        return false;
    return static_cast<JsonUnsignedInteger>(si) == u.access<JsonUnsignedInteger>();
}

bool isJsonNumber(JsonType t) {
    return t >= JsonType::SignedInteger && t <= JsonType::Float;
}

} // namespace

bool operator==(const Json& x, const Json& y) {
    JsonType xt = x.type();
    JsonType yt = y.type();
    if (isJsonNumber(xt) && isJsonNumber(yt)) {
        if (xt == yt) {
            // Same type: compare directly.
            switch (xt) {
            case JsonType::SignedInteger:
                return x.access<JsonSignedInteger>() == y.access<JsonSignedInteger>();
            case JsonType::UnsignedInteger:
                return x.access<JsonUnsignedInteger>() == y.access<JsonUnsignedInteger>();
            default:
                return x.access<JsonFloat>() == y.access<JsonFloat>();
            }
        }
        if (xt == JsonType::Float)
            return floatEqualsInt(x.access<JsonFloat>(), y);
        if (yt == JsonType::Float)
            return floatEqualsInt(y.access<JsonFloat>(), x);
        // SignedInteger vs UnsignedInteger
        if (xt == JsonType::SignedInteger)
            return signedEqualsUnsigned(x, y);
        else
            return signedEqualsUnsigned(y, x);
    } else {
        return operator==(static_cast<const JsonVariant&>(x), static_cast<const JsonVariant&>(y));
    }
}

bool operator!=(const Json& x, const Json& y) {
    return !operator==(x, y);
}

} // namespace Brisk
