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
#pragma once
#include <type_traits>
#include <atomic>
#include <vector>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <variant>
#include <cstdint>
#include "internal/Optional.hpp"
#include <brisk/core/Bytes.hpp>
#include "Reflection.hpp"

namespace Brisk {

struct Json;

using JsonArray           = std::vector<Json>;
using JsonObject          = std::map<std::string, Json, std::less<>>;
using JsonString          = std::string;
using JsonSignedInteger   = int64_t;
using JsonUnsignedInteger = uint64_t;
using JsonFloat           = double;
using JsonNull            = std::nullptr_t;
using JsonBool            = bool;

/**
 * @enum JsonType
 * @brief Enum representing the different types of JSON values.
 *
 * This enum class defines the various data types that can be found in a JSON structure.
 * Each value corresponds to a specific JSON data type.
 */
enum class JsonType {
    /**
     * @brief Represents a JSON null value.
     */
    Null,

    /**
     * @brief Represents a JSON array.
     */
    Array,

    /**
     * @brief Represents a JSON object.
     */
    Object,

    /**
     * @brief Represents a JSON string.
     */
    String,

    /**
     * @brief Represents a JSON signed integer (64-bit).
     */
    SignedInteger,

    /**
     * @brief Represents a JSON unsigned integer (64-bit).
     */
    UnsignedInteger,

    /**
     * @brief Represents a JSON floating-point number (64-bit).
     */
    Float,

    /**
     * @brief Represents a JSON boolean value.
     */
    Bool,

    Last = Bool,
};

template <>
inline constexpr std::initializer_list<NameValuePair<JsonType>> defaultNames<JsonType>{
    { "Null", JsonType::Null },
    { "Array", JsonType::Array },
    { "Object", JsonType::Object },
    { "String", JsonType::String },
    { "SignedInteger", JsonType::SignedInteger },
    { "UnsignedInteger", JsonType::UnsignedInteger },
    { "Float", JsonType::Float },
    { "Bool", JsonType::Bool },
};

struct Json;

template <typename T>
bool adlToJson(Json& b, const T& v);

template <typename T>
bool adlFromJson(const Json& b, T& v);

template <typename T>
struct JsonConverter {
    static bool toJson(Json& b, const T& v) {
        return adlToJson(b, v);
    }

    static bool fromJson(const Json& b, T& v) {
        return adlFromJson(b, v);
    }
};

/**
 * @typedef JsonVariant
 * @brief A variant type that can represent any JSON value. Used by `Json` for storage.
 */
using JsonVariant = std::variant<JsonNull, JsonArray, JsonObject, JsonString, JsonSignedInteger,
                                 JsonUnsignedInteger, JsonFloat, JsonBool>;
using JsonTypes = TypeIDs<JsonNull, JsonArray, JsonObject, JsonString, JsonSignedInteger, JsonUnsignedInteger,
                          JsonFloat, JsonBool>;

template <typename T, typename... V>
constexpr size_t findType(TypeIDs<>) {
    return 0;
}

template <typename T, typename V0, typename... V>
constexpr size_t findType(TypeIDs<V0, V...>) {
    if (std::is_same_v<T, V0>)
        return 0;
    return 1 + findType<T>(TypeIDs<V...>{});
}

template <typename T>
constexpr bool isJsonType() {
    if constexpr (std::is_same_v<T, Json>)
        return true;
    else
        return findType<T>(JsonTypes{}) <= static_cast<int>(JsonType::Last);
}

template <typename T>
constexpr bool isJsonCompoundType() {
    if constexpr (std::is_same_v<T, Json>)
        return true;
    else
        return findType<T>(JsonTypes{}) <= static_cast<int>(JsonType::String);
}

/**
 * @struct IteratePath
 * @brief A structure to iterate over a JSON path.
 *
 * This structure provides functionality to parse and iterate over a specified path
 * within a JSON document, enabling access to nested elements.
 */
struct IteratePath {
    /** @brief The path to iterate over. */
    std::string_view path;

    /**
     * @struct Iterator
     * @brief An iterator for IteratePath.
     *
     * This struct implements a forward iterator for traversing the components of a JSON path.
     */
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::string_view;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        std::string_view path;
        size_t cur  = 0;
        size_t next = 0;

        /**
         * @brief Compares two iterators for equality.
         * @param it The iterator to compare against.
         * @return True if both iterators point to the same element; otherwise, false.
         */
        bool operator==(const Iterator& it) const;

        /**
         * @brief Compares two iterators for inequality.
         * @param it The iterator to compare against.
         * @return True if the iterators point to different elements; otherwise, false.
         */
        bool operator!=(const Iterator& it) const;

        /**
         * @brief Dereferences the iterator to get the current path component.
         * @return The current path component as a string view.
         */
        std::string_view operator*() const;

        /**
         * @brief Advances the iterator to the next path component.
         * @return A reference to this iterator.
         */
        Iterator& operator++();

        /**
         * @brief Advances the iterator to the next path component (postfix increment).
         * @return A copy of the iterator before incrementing.
         */
        Iterator operator++(int);
    };

    /**
     * @brief Returns an iterator to the beginning of the path components.
     * @return An Iterator pointing to the first component.
     */
    Iterator begin() const;

    /**
     * @brief Finds the next delimiter in the path.
     * @param path The path to search.
     * @param offset The starting position for the search.
     * @return The position of the next delimiter.
     */
    static size_t find_next(std::string_view path, size_t offset);

    /**
     * @brief Returns an iterator to the end of the path components.
     * @return An Iterator pointing to the end.
     */
    Iterator end() const;
};

/**
 * @brief Parses a string and returns an IteratePath structure.
 * @param s The string representing the JSON path.
 * @return An IteratePath object representing the parsed path.
 */
IteratePath iteratePath(std::string_view s);

/**
 * @struct Json
 * @brief A structure representing a JSON value.
 *
 * This structure encapsulates various types of JSON data and provides methods for
 * manipulation and conversion between JSON and other data types.
 */
struct Json : protected JsonVariant {
    using JsonVariant::JsonVariant; ///< Inherit constructors from JsonVariant.

    /** @brief Constructs a Json object from a JsonArray. */
    Json(JsonArray x) : JsonVariant(std::move(x)) {}

    /** @brief Constructs a Json object from a JsonObject. */
    Json(JsonObject x) : JsonVariant(std::move(x)) {}

    /** @brief Constructs a Json object from a JsonString. */
    Json(JsonString x) : JsonVariant(std::move(x)) {}

    /** @brief Constructs a Json object from a nullptr (JSON null). */
    Json(std::nullptr_t x) : JsonVariant(static_cast<JsonNull>(x)) {}

    /** @brief Constructs a Json object from a boolean value. */
    Json(bool x) : JsonVariant(static_cast<JsonBool>(x)) {}

    /** @brief Constructs a Json object from a C-style string. */
    Json(const char* x) : JsonVariant(JsonString(x)) {}

    /** @brief Constructs a Json object from a string view. */
    Json(std::string_view x) : JsonVariant(JsonString(x)) {}

    /** @brief Constructs a Json object from a float value. */
    Json(float x) : JsonVariant(static_cast<JsonFloat>(x)) {}

    /** @brief Constructs a Json object from a double value. */
    Json(double x) : JsonVariant(static_cast<JsonFloat>(x)) {}

    /** @brief Constructs a Json object from a character. */
    Json(char x)
        : JsonVariant(
              static_cast<std::conditional_t<std::is_signed_v<char>, JsonSignedInteger, JsonUnsignedInteger>>(
                  x)) {}

    /** @brief Constructs a Json object from a signed char. */
    Json(signed char x) : JsonVariant(static_cast<JsonSignedInteger>(x)) {}

    /** @brief Constructs a Json object from a signed short. */
    Json(signed short x) : JsonVariant(static_cast<JsonSignedInteger>(x)) {}

    /** @brief Constructs a Json object from a signed int. */
    Json(signed int x) : JsonVariant(static_cast<JsonSignedInteger>(x)) {}

    /** @brief Constructs a Json object from a signed long. */
    Json(signed long x) : JsonVariant(static_cast<JsonSignedInteger>(x)) {}

    /** @brief Constructs a Json object from a signed long long. */
    Json(signed long long x) : JsonVariant(static_cast<JsonSignedInteger>(x)) {}

    /** @brief Constructs a Json object from an unsigned char. */
    Json(unsigned char x) : JsonVariant(static_cast<JsonUnsignedInteger>(x)) {}

    /** @brief Constructs a Json object from an unsigned short. */
    Json(unsigned short x) : JsonVariant(static_cast<JsonUnsignedInteger>(x)) {}

    /** @brief Constructs a Json object from an unsigned int. */
    Json(unsigned int x) : JsonVariant(static_cast<JsonUnsignedInteger>(x)) {}

    /** @brief Constructs a Json object from an unsigned long. */
    Json(unsigned long x) : JsonVariant(static_cast<JsonUnsignedInteger>(x)) {}

    /** @brief Constructs a Json object from an unsigned long long. */
    Json(unsigned long long x) : JsonVariant(static_cast<JsonUnsignedInteger>(x)) {}

    /** @brief Copy constructor. */
    Json(const Json&)            = default;

    /** @brief Move constructor. */
    Json(Json&&)                 = default;

    /** @brief Copy assignment operator. */
    Json& operator=(const Json&) = default;

    /** @brief Move assignment operator. */
    Json& operator=(Json&&)      = default;

    /**
     * @brief Constructs a Json object from any value type.
     * @tparam U The type of the value.
     * @param val The value to construct the Json object from.
     */
    template <typename U>
    Json(U val) : JsonVariant(nullptr) {
        if (!JsonConverter<U>::toJson(*this, val))
            *this = nullptr;
    }

    /**
     * @brief Checks if the current Json value is of a specific type.
     * @tparam T The type to check against.
     * @return True if the current value is of type T; otherwise, false.
     */
    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(*this);
    }

    /**
     * @brief Returns the JsonType of the current value.
     * @return The type of the current Json value.
     */
    JsonType type() const {
        return static_cast<JsonType>(index());
    }

    /**
     * @brief Accesses the value of type T from the Json object.
     * @tparam T The type to access.
     * @return A const reference to the value of type T.
     */
    template <typename T>
    const T& access() const& {
        static_assert(isJsonType<T>());
        return std::get<T>(*this);
    }

    /**
     * @brief Accesses the value of type T from the Json object.
     * @tparam T The type to access.
     * @return A reference to the value of type T.
     */
    template <typename T>
    T& access() & {
        static_assert(isJsonType<T>());
        return std::get<T>(*this);
    }

    /**
     * @brief Accesses the value of type T from the Json object.
     * @tparam T The type to access.
     * @return An rvalue reference to the value of type T.
     */
    template <typename T>
    T&& access() && {
        static_assert(isJsonType<T>());
        return std::get<T>(std::move(*this));
    }

    template <typename T>
    using ConstRefOpt = std::conditional_t<isJsonCompoundType<T>(), optional_ref<const T>, optional<T>>;
    template <typename T>
    using RefOpt = std::conditional_t<isJsonCompoundType<T>(), optional_ref<T>, optional<T>>;

    /**
     * @brief Converts the current Json value to another type.
     * @tparam T The target type.
     * @return An optional reference to type T, if the conversion is successful.
     */
    template <typename T>
    RefOpt<T> to() {
        if constexpr (std::is_same_v<T, Json>) {
            return *this;
        } else if constexpr (isJsonCompoundType<T>()) {
            if (is<T>())
                return access<T>();
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (is<JsonFloat>())
                return static_cast<T>(access<JsonFloat>());
            if (is<JsonSignedInteger>())
                return static_cast<T>(access<JsonSignedInteger>());
            else if (is<JsonUnsignedInteger>())
                return static_cast<T>(access<JsonUnsignedInteger>());
            else if (is<JsonBool>())
                return static_cast<T>(access<JsonBool>());
        } else {
            T val{};
            if (JsonConverter<T>::fromJson(*this, val))
                return val;
        }
        return nullopt;
    }

    /**
     * @brief Converts the current Json value to another type.
     * @tparam T The target type.
     * @return An optional value of type T, if the conversion is successful.
     */
    template <typename T>
    ConstRefOpt<T> to() const {
        if constexpr (std::is_same_v<T, Json>) {
            return *this;
        } else if constexpr (isJsonCompoundType<T>()) {
            if (is<T>())
                return access<T>();
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (is<JsonFloat>())
                return static_cast<T>(access<JsonFloat>());
            if (is<JsonSignedInteger>())
                return static_cast<T>(access<JsonSignedInteger>());
            else if (is<JsonUnsignedInteger>())
                return static_cast<T>(access<JsonUnsignedInteger>());
            else if (is<JsonBool>())
                return static_cast<T>(access<JsonBool>());
        } else {
            T val{};
            if (JsonConverter<T>::fromJson(*this, val))
                return val;
        }
        return nullopt;
    }

    /**
     * @brief Converts the current Json value to another type and stores it in the provided variable.
     * @tparam T The target type.
     * @param val The variable to store the converted value.
     * @return True if the conversion was successful; otherwise, false.
     */
    template <typename T>
    bool to(T& val) const {
        if constexpr (std::is_same_v<T, Json>) {
            val = *this;
            return true;
        } else if constexpr (isJsonCompoundType<T>()) {
            if (is<T>()) {
                val = access<T>();
                return true;
            }
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (is<JsonFloat>())
                val = static_cast<T>(access<JsonFloat>());
            if (is<JsonSignedInteger>())
                val = static_cast<T>(access<JsonSignedInteger>());
            else if (is<JsonUnsignedInteger>())
                val = static_cast<T>(access<JsonUnsignedInteger>());
            else if (is<JsonBool>())
                val = static_cast<T>(access<JsonBool>());
            else
                return false;
            return true;
        } else {
            if (JsonConverter<T>::fromJson(*this, val))
                return true;
        }
        return false;
    }

    /**
     * @brief Converts the current Json object to a JSON string representation.
     * @param indent The number of spaces for indentation (default is 0). If negative, tabs will be
     * used instead of spaces.
     * @return A string representing the JSON.
     */
    std::string toJson(int indent = 0) const;

    /**
     * @brief Parses a JSON string and returns a Json object.
     * @param s The JSON string to parse.
     * @return An optional Json object if parsing is successful.
     */
    static optional<Json> fromJson(const std::string& s);

    /**
     * @brief Converts the current Json object to a MessagePack byte array.
     * @return A vector of bytes representing the MessagePack format.
     */
    Bytes toMsgPack() const;

    /**
     * @brief Parses a MessagePack byte array and returns a Json object.
     * @param s The byte array to parse.
     * @return An optional Json object if parsing is successful.
     */
    static optional<Json> fromMsgPack(const BytesView& s);

    /**
     * @brief Compares two Json objects for equality.
     * @param x The first Json object.
     * @param y The second Json object.
     * @return True if both Json objects are equal; otherwise, false.
     */
    friend bool operator==(const Json& x, const Json& y);

    /**
     * @brief Compares two Json objects for inequality.
     * @param x The first Json object.
     * @param y The second Json object.
     * @return True if both Json objects are not equal; otherwise, false.
     */
    friend bool operator!=(const Json& x, const Json& y);

    /**
     * @brief Retrieves an item from a JsonObject by key.
     * @tparam T The type of the item.
     * @param key The key of the item to retrieve.
     * @return An optional reference to the item of type T if it exists.
     */
    template <typename T = Json>
    ConstRefOpt<T> getItem(std::string_view key) const {
        optional_ref<const JsonObject> o = to<JsonObject>();
        if (!o.has_value())
            return nullopt;
        auto it = o->find(key);
        if (it == o->end())
            return nullopt;
        return it->second.template to<T>();
    }

    /**
     * @brief Retrieves an item from a JsonObject by key and stores it in the provided variable.
     * @tparam T The type of the item.
     * @param key The key of the item to retrieve.
     * @param to The variable to store the item.
     * @return True if the item exists and was retrieved successfully; otherwise, false.
     */
    template <typename T = Json>
    bool getItemTo(std::string_view key, T& to) const {
        optional_ref<const JsonObject> o = this->to<JsonObject>();
        if (!o.has_value())
            return false;
        auto it = o->find(key);
        if (it == o->end())
            return false;
        return it->second.to(to);
    }

    /**
     * @brief Checks if an item exists in the JsonObject by key.
     * @param key The key of the item to check.
     * @return True if the item exists; otherwise, false.
     */
    bool exists(std::string_view key) const {
        optional_ref<const JsonObject> o = this->to<JsonObject>();
        if (!o.has_value())
            return false;
        auto it = o->find(key);
        if (it == o->end())
            return false;
        return true;
    }

    /**
     * @brief Sets an item in a JsonObject by key.
     * @tparam T The type of the value to set.
     * @param key The key of the item to set.
     * @param val The value to assign to the item.
     * @return True if the item was set successfully; otherwise, false.
     */
    template <typename T>
    bool setItem(std::string_view key, T&& val) {
        toObject();
        optional_ref<JsonObject> o = to<JsonObject>();
        if (!o.has_value())
            return false;
        Json b;
        if (!JsonConverter<std::decay_t<T>>::toJson(b, std::forward<T>(val)))
            return false;
        auto it = o->find(key);
        if (it == o->end())
            o->insert_or_assign(std::string(key), std::move(b));
        else
            it->second = std::move(b);
        return true;
    }

    /**
     * @brief Retrieves an item from the Json object by path.
     * @tparam T The type of the item.
     * @param path The path to the item.
     * @return An optional reference to the item of type T if it exists.
     */
    template <typename T = Json>
    ConstRefOpt<T> itemByPath(std::string_view path) const {
        const Json* root = this;
        for (std::string_view key : iteratePath(path)) {
            if (key.empty())
                continue;
            optional_ref<const JsonObject> o = root->to<JsonObject>();
            if (!o)
                return nullopt;
            auto it = o->find(key);
            if (it == o->end())
                return nullopt;
            root = &it->second;
        }
        if (!root) // empty path
            return nullopt;
        return root->template to<T>();
    }

    /**
     * @brief Sets an item in the Json object by path.
     * @tparam T The type of the value to set.
     * @param path The path to the item.
     * @param val The value to assign to the item.
     * @return True if the item was set successfully; otherwise, false.
     */
    template <typename T>
    bool setItemByPath(std::string_view path, T&& val) {
        toObject();
        Json b;
        if (!JsonConverter<std::decay_t<T>>::toJson(b, std::forward<T>(val)))
            return false;

        Json* root = this;
        for (std::string_view key : iteratePath(path)) {
            if (key.empty())
                continue;
            optional_ref<JsonObject> o = root->to<JsonObject>();
            if (!o)
                return false;
            auto it = o->find(key);
            if (it == o->end()) {
                // create new key
                it = o->insert_or_assign(std::string(key), JsonObject{}).first;
            }
            root = &it->second;
        }
        if (!root) // empty path
            return false;
        std::swap(b, *root);
        return true;
    }

    template <typename T>
    bool from(T&& val) {
        Json b;
        if (JsonConverter<std::decay_t<T>>::toJson(b, std::forward<T>(val))) {
            std::swap(*this, b);
            return true;
        }
        return false;
    }

    /**
     * @brief Returns the number of elements in the Json object or array.
     *
     * Returns 0 if the Json value is neither an object nor an array
     *
     * @return The size of the Json object.
     */
    size_t size() const {
        switch (type()) {
        case JsonType::Array:
            return access<JsonArray>().size();
        case JsonType::Object:
            return access<JsonObject>().size();
        default:
            return 0;
        }
    }

    /**
     * @brief Ensures that the current Json value is an object. If not, initializes it as an empty JsonObject.
     * @return A reference to the current Json object.
     */
    Json& toObject() {
        if (type() != JsonType::Object) {
            *this = JsonObject{};
        }
        return *this;
    }
};

template <typename T>
    requires(isJsonCompoundType<T>())
struct JsonConverter<T> {
    static bool toJson(Json& b, T v) {
        b = v;
        return true;
    }

    static bool fromJson(const Json& b, T& v) {
        v = b.access<T>();
        return true;
    }
};

template <typename T>
    requires(std::is_arithmetic_v<T> && !isJsonCompoundType<T>())
struct JsonConverter<T> {
    static bool toJson(Json& b, T v) {
        if constexpr (std::is_floating_point_v<T>)
            b = JsonFloat(v);
        else if constexpr (std::is_signed_v<T>)
            b = JsonSignedInteger(v);
        else if constexpr (std::is_same_v<T, bool>)
            b = JsonBool(v);
        else
            b = JsonUnsignedInteger(v);
        return true;
    }

    static bool fromJson(const Json& b, T& v) {
        if (b.is<JsonFloat>()) {
            v = b.access<JsonFloat>();
            return true;
        } else if (b.is<JsonSignedInteger>()) {
            v = b.access<JsonSignedInteger>();
            return true;
        } else if (b.is<JsonUnsignedInteger>()) {
            v = b.access<JsonUnsignedInteger>();
            return true;
        } else if (b.is<JsonBool>()) {
            v = b.access<JsonBool>();
            return true;
        }
        return false;
    }
};

template <typename T>
inline bool toJson(Json& b, T v)
    requires std::is_enum_v<T>
{
    b = static_cast<std::underlying_type_t<T>>(v);
    return true;
}

template <typename T>
inline bool fromJson(const Json& b, T& v)
    requires std::is_enum_v<T>
{
    return b.to(reinterpret_cast<std::underlying_type_t<T>&>(v));
}

template <typename T, typename U>
inline bool assignOpt(T& dst, const optional<U>& src) {
    if (src.has_value()) {
        dst = *src;
    }
    return src.has_value();
}

/**
 * @brief Packs a variadic list of arguments into a JsonArray.
 *
 * This function takes a variadic list of elements and constructs a JsonArray
 * containing those elements. The elements are perfectly forwarded to ensure
 * that their types are preserved (e.g., lvalues remain lvalues).
 *
 * @tparam T The types of the arguments to pack.
 * @param b The Json object to pack into, which will be assigned a new JsonArray.
 * @param args The variadic list of arguments to pack into the JsonArray.
 * @return True, indicating that packing was successful.
 */
template <typename... T>
inline bool packArray(Json& b, T&&... args) {
    b = JsonArray{ std::forward<T>(args)... };
    return true;
}

/**
 * @brief Unpacks the elements of a Json array into a variadic list of arguments.
 *
 * This function attempts to extract elements from a given Json object (expected to be an array)
 * and assign them to the provided arguments. The number of elements in the JsonArray must be
 * at least equal to the number of provided arguments.
 *
 * @tparam T Types of the arguments to unpack into.
 * @param b The Json object to unpack from.
 * @param args The variadic list of arguments to unpack into.
 * @return True if unpacking was successful; otherwise, false.
 */
template <typename... T>
inline bool unpackArray(const Json& b, T&... args) {
    if (optional_ref<const JsonArray> a = b.to<JsonArray>()) {
        if (a->size() >= sizeof...(T)) {
            size_t i = 0; // sequence guaranteed
            return (assignOpt(args, (*a)[i++].to<T>()) && ...);
        }
    }
    return false;
}

/**
 * @brief Packs a fixed-size array into a JsonArray.
 *
 * This function takes a fixed-size array and assigns its elements to a Json object
 * represented as a JsonArray.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The number of elements in the array.
 * @param b The Json object to pack into.
 * @param args The fixed-size array of elements to pack.
 * @return True, indicating that packing was successful.
 */
template <typename T, size_t N>
inline bool packArray(Json& b, const T (&args)[N]) {
    b = JsonArray(std::begin(args), std::end(args));
    return true;
}

/**
 * @brief Unpacks the elements of a JsonArray into a fixed-size array.
 *
 * This function extracts elements from a given Json object (expected to be an array)
 * and assigns them to a fixed-size array. The size of the JsonArray must be at least
 * equal to the size of the provided array.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The number of elements in the array.
 * @param b The Json object to unpack from.
 * @param args The fixed-size array to unpack into.
 * @return True if unpacking was successful; otherwise, false.
 */
template <typename T, size_t N>
inline bool unpackArray(const Json& b, T (&args)[N]) {
    if (optional_ref<const JsonArray> a = b.to<JsonArray>()) {
        if (a->size() >= N) {
            for (size_t i = 0; i < N; ++i) {
                if (!assignOpt(args[i], (*a)[i].to<T>()))
                    return false;
            }
            return true;
        }
    }
    return false;
}

template <typename T, size_t N>
inline bool toJson(Json& b, const std::array<T, N>& v) {
    JsonArray a(v.size());

    for (size_t i = 0; i < v.size(); ++i) {
        if (!JsonConverter<T>::toJson(a[i], v[i]))
            return false;
    }

    b = std::move(a);
    return true;
}

template <typename T, size_t N>
inline bool fromJson(const Json& b, std::array<T, N>& v) {
    if (!b.is<JsonArray>())
        return false;
    const JsonArray& a = b.access<JsonArray>();
    if (a.size() < N)
        return false;
    std::array<T, N> temp;
    for (size_t i = 0; i < N; ++i) {
        if (!JsonConverter<T>::fromJson(a[i], temp[i]))
            return false;
    }
    std::swap(temp, v);
    return true;
}

template <typename K, typename C, typename Alloc>
inline bool toJson(Json& b, const std::set<K, C, Alloc>& v) {
    JsonArray a(v.size());

    size_t i = 0;
    for (auto it = v.begin(); it != v.end(); ++i, ++it) {
        if (!JsonConverter<K>::toJson(a[i], *it))
            return false;
    }

    b = std::move(a);
    return true;
}

template <typename K, typename C, typename Alloc>
inline bool fromJson(const Json& b, std::set<K, C, Alloc>& v) {
    if (!b.is<JsonArray>())
        return false;
    const JsonArray& a = b.access<JsonArray>();
    std::set<K, C, Alloc> temp;
    for (size_t i = 0; i < a.size(); ++i) {
        K val;
        if (!JsonConverter<K>::fromJson(a[i], val))
            return false;
        temp.insert(val);
    }
    std::swap(temp, v);
    return true;
}

template <typename T, typename Alloc>
inline bool toJson(Json& b, const std::vector<T, Alloc>& v) {
    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t> || std::is_same_v<T, std::byte>) {
        b = toHex(toBytesView(v));
        return true;
    } else {
        JsonArray a(v.size());

        for (size_t i = 0; i < v.size(); ++i) {
            if (!JsonConverter<T>::toJson(a[i], v[i]))
                return false;
        }

        b = std::move(a);
        return true;
    }
}

template <typename T, typename Alloc>
inline bool fromJson(const Json& b, std::vector<T, Alloc>& v) {
    if (b.is<JsonArray>()) {
        const JsonArray& a = b.access<JsonArray>();
        std::vector<T, Alloc> temp(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            if (!JsonConverter<T>::fromJson(a[i], temp[i]))
                return false;
        }
        std::swap(temp, v);
        return true;
    }

    if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t> || std::is_same_v<T, std::byte>) {
        if (b.is<JsonString>()) {
            const JsonString& s = b.access<JsonString>();
            if (s.size() & 1)
                return false;
            std::vector<T, Alloc> temp(s.size() / 2);
            if (fromHex(temp, s) != temp.size())
                return false;
            std::swap(temp, v);
            return true;
        }
    }

    return false;
}

template <typename K, typename V>
inline bool toJson(Json& b, const std::pair<K, V>& v) {
    return packArray(b, v.first, v.second);
}

template <typename K, typename V>
inline bool fromJson(const Json& b, std::pair<K, V>& v) {
    return unpackArray(b, v.first, v.second);
}

template <typename K, typename V, typename C, typename Alloc>
inline bool toJson(Json& b, const std::map<K, V, C, Alloc>& v) {
    JsonArray a(v.size());

    size_t i = 0;
    for (const std::pair<const K, V>& p : v) {
        if (!a[i].from(p))
            return false;
        ++i;
    }

    b = std::move(a);
    return true;
}

template <typename K, typename V, typename C, typename Alloc>
inline bool fromJson(const Json& b, std::map<K, V, C, Alloc>& v) {
    if (!b.is<JsonArray>())
        return false;
    const JsonArray& a = b.access<JsonArray>();
    std::map<K, V, C, Alloc> temp;
    for (size_t i = 0; i < a.size(); ++i) {
        std::pair<K, V> p;
        if (!a[i].to(p))
            return false;
        temp.insert_or_assign(std::move(p.first), std::move(p.second));
    }
    std::swap(temp, v);
    return true;
}

template <typename T>
inline bool adlToJson(Json& b, const T& v) {
    return toJson(b, v);
}

template <typename T>
inline bool adlFromJson(const Json& b, T& v) {
    return fromJson(b, v);
}

template <size_t N>
inline bool fromJson(const Json& j, FixedBytes<N>& p) {
    if (auto s = j.to<std::string>()) {
        if (s->size() != N * 2)
            return false;
        if (auto pp = FixedBytes<N>::fromHex(*s)) {
            p = *pp;
            return true;
        }
        return false;
    }
    return false;
}

template <size_t N>
inline bool toJson(Json& j, const FixedBytes<N>& p) {
    j = p.toHex();
    return true;
}

namespace Internal {

template <typename T>
bool reflectToJson(size_constants<>, Json& j, const T& val, const std::tuple<>& fields) {
    return true;
}

template <size_t... indices, typename T, typename Class, typename... FieldType>
bool reflectToJson(size_constants<indices...>, Json& j, const T& val,
                   const std::tuple<ReflectionField<Class, FieldType>...>& fields) {
    ((!(std::get<indices>(fields).flags && ReflectionFlag::SkipSerialization)
      ? j.setItem(std::get<indices>(fields).name, val.*(std::get<indices>(fields).pointerToField)),
      void() : void()),
     ...);
    return true;
}

template <typename T>
bool reflectFromJson(size_constants<>, const Json& j, T& val, const std::tuple<>& fields) {
    return true;
}

template <size_t... indices, typename T, typename Class, typename... FieldType>
bool reflectFromJson(size_constants<indices...>, const Json& j, T& val,
                     const std::tuple<ReflectionField<Class, FieldType>...>& fields) {
    if (j.type() == JsonType::Object) {
        ((!(std::get<indices>(fields).flags && ReflectionFlag::SkipSerialization)
          ? j.getItemTo(std::get<indices>(fields).name, val.*(std::get<indices>(fields).pointerToField)),
          void() : void()),
         ...);
        return true;
    }
    return false;
}

} // namespace Internal

/**
 * @brief Specialization of `JsonConverter` for types that have reflection metadata.
 *
 * Provides functions to serialize and deserialize objects with reflection metadata to/from JSON.
 *
 * @tparam T The type of the object being converted to/from JSON.
 */
template <HasReflection T>
struct JsonConverter<T> {
    constexpr static auto numFields = std::tuple_size_v<decltype(T::Reflection)>;

    /**
     * @brief Serializes an object to a JSON object using reflection.
     *
     * @param j The JSON object to populate.
     * @param val The object to serialize.
     * @return `true` if serialization succeeds.
     */
    static bool toJson(Json& j, const T& val) {
        return Internal::reflectToJson(size_sequence<numFields>{}, j, val, T::Reflection);
    }

    /**
     * @brief Deserializes an object from a JSON object using reflection.
     *
     * @param j The JSON object to read from.
     * @param val The object to populate with data from the JSON object.
     * @return `true` if deserialization succeeds.
     */
    static bool fromJson(const Json& j, T& val) {
        return Internal::reflectFromJson(size_sequence<numFields>{}, j, val, T::Reflection);
    }
};

/**
 * @brief Specialization of `JsonConverter` for empty types.
 *
 * Provides functions to serialize and deserialize empty types to/from JSON objects.
 *
 * @tparam T The empty type being converted to/from JSON.
 */
template <typename T>
    requires(std::is_empty_v<T>)
struct JsonConverter<T> {

    /**
     * @brief Serializes an empty object to a JSON object.
     *
     * @param j The JSON object to populate (empty object).
     * @param val The object to serialize (empty, not used).
     * @return Always returns `true`.
     */
    static bool toJson(Json& j, const T& val) {
        j = JsonObject{};
        return true;
    }

    /**
     * @brief Deserializes an empty object from a JSON object.
     *
     * @param j The JSON object to read from.
     * @param val The object to populate (empty, not modified).
     * @return Always returns `true`.
     */
    static bool fromJson(const Json& j, T& val) {
        return true;
    }
};

template <typename T>
inline bool fromJson(const Json& j, std::atomic<T>& p) {
    if (auto v = j.to<T>()) {
        p.store(*v);
        return true;
    }
    return false;
}

template <typename T>
inline bool toJson(Json& j, const std::atomic<T>& p) {
    return j.from(p.load());
}

} // namespace Brisk
