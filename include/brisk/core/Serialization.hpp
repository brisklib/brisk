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
#include "BasicTypes.hpp"
#include <brisk/core/Binding.hpp>
#include "internal/Function.hpp"
#include "Reflection.hpp"
#include <atomic>
#include "Json.hpp"

namespace Brisk {

/**
 * @brief Enum representing the action performed during serialization.
 */
enum class SerializationAction {
    Load, ///< Load data during deserialization.
    Save, ///< Save data during serialization.
};

/**
 * @brief Interface for serializable objects.
 */
class SerializableInterface;

/**
 * @brief Handles serialization and deserialization of data to and from JSON.
 *
 * The `Serialization` struct provides functionality to perform both loading (deserialization) and
 * saving (serialization) of object data. It interacts with a `Json` object to store and retrieve values.
 */
struct Serialization final {
    SerializationAction action; ///< The current serialization action (load or save).
    mutable Json data;          ///< The JSON data being processed.
    function<void(Json&&)>
        callback; ///< Optional callback invoked at destruction, useful for nested serialization contexts.

    /// Default constructor.
    Serialization()                     = default;

    /// Copy constructor.
    Serialization(const Serialization&) = default;

    /// Move constructor.
    Serialization(Serialization&&)      = default;

    /**
     * @brief Destructor, invokes the callback with the processed JSON data.
     */
    ~Serialization() {
        if (callback)
            callback(std::move(data));
    }

    /**
     * @brief Logs an error during serialization.
     */
    void serializationError() const {}

    /**
     * @brief Logs an error during deserialization.
     */
    void deserializationError() const {}

    /**
     * @brief Processes a property of a specific type for serialization or deserialization.
     *
     * If the current action is loading, it tries to retrieve the value from the JSON.
     * If saving, it stores the value into the JSON.
     *
     * @tparam Type The type of the property.
     * @param prop The property to process.
     * @param name The JSON key associated with the property.
     */
    template <typename Type>
    void operator()(const Value<Type>& prop, std::string_view name) const {
        if (action == SerializationAction::Load) {
            if (auto val = data.getItem<Type>(name)) {
                prop.set(std::move(*val));
            } else {
                deserializationError();
            }
        } else {
            if (!data.setItem(name, prop.get())) {
                serializationError();
            }
        }
    }

    /**
     * @brief Processes a value with a setter function for serialization or deserialization.
     *
     * Useful when custom setting logic is required during loading.
     *
     * @tparam Value The type of the value being processed.
     * @tparam Setter The setter function to apply when loading data.
     * @param value The value to process.
     * @param setter The setter function for the value.
     * @param name The JSON key associated with the value.
     */
    template <typename Value, typename Setter>
    void operator()(Value& value, Setter&& setter, std::string_view name) const {
        static_assert(!std::is_const_v<Value>);
        if (action == SerializationAction::Load) {
            if (auto val = data.getItem<Value>(name)) {
                setter(std::move(*val));
            } else {
                deserializationError();
            }
        } else {
            if (!data.setItem(name, value)) {
                serializationError();
            }
        }
    }

    /**
     * @brief Processes a value for serialization or deserialization.
     *
     * Handles simple types without the need for a property wrapper.
     *
     * @tparam Value The type of the value being processed.
     * @param value The value to process.
     * @param name The JSON key associated with the value.
     */
    template <typename Value>
    void operator()(Value& value, std::string_view name) const {
        static_assert(!std::is_const_v<Value>);
        if (action == SerializationAction::Load) {
            if (!data.getItemTo<Value>(name, value)) {
                deserializationError();
            }
        } else {
            if (!data.setItem(name, value)) {
                serializationError();
            }
        }
    }

    /**
     * @brief Processes a serializable object.
     *
     * Calls the `serialize` method on the object, allowing it to handle its own serialization.
     *
     * @param value The serializable object to process.
     * @param name The JSON key associated with the object.
     */
    void operator()(SerializableInterface& value, std::string_view name) const;

    /**
     * @brief Returns a new `Serialization` context for a nested JSON object under a specific key.
     *
     * Allows handling of nested serialization for objects contained within another object.
     *
     * @param name The key for the nested object in the JSON data.
     * @return A new `Serialization` object for the nested context.
     */
    Serialization key(std::string_view name) const {
        if (data.type() == JsonType::Null)
            data = JsonObject();
        BRISK_ASSERT(data.type() == JsonType::Object);
        Serialization result = *this;
        result.data          = data.getItem<Json>(name).value_or(Json{});
        result.callback      = [this, name](Json&& nestedJson) BRISK_INLINE_LAMBDA {
            this->data.setItem(name, std::move(nestedJson));
        };
        return result;
    }

    /**
     * @brief Returns a new `Serialization` context for a specific index in a JSON array.
     *
     * Allows handling of nested serialization for arrays.
     *
     * @param idx The index of the array to process.
     * @return A new `Serialization` object for the nested array context.
     */
    Serialization index(int idx) const {
        if (data.type() == JsonType::Null)
            data = JsonArray();
        BRISK_ASSERT(data.type() == JsonType::Array);
        JsonArray& arr       = data.access<JsonArray>();
        Serialization result = *this;
        result.data          = idx < arr.size() ? arr[idx] : Json{};
        result.callback      = [this, idx](Json&& nestedJson) BRISK_INLINE_LAMBDA {
            BRISK_ASSERT(this->data.type() == JsonType::Array);
            JsonArray& arr = this->data.access<JsonArray>();
            BRISK_ASSERT(idx <= arr.size());
            if (idx < arr.size())
                arr[idx] = std::move(nestedJson);
            else if (idx == arr.size())
                arr.push_back(std::move(nestedJson));
        };
        return result;
    }
};

/**
 * @brief Interface class for objects that can be serialized.
 *
 * Implement this interface for custom objects that need to define how they are serialized and deserialized.
 */
class SerializableInterface {
public:
    /**
     * @brief Virtual method to be overridden by derived classes for custom serialization.
     *
     * @param serialization The `Serialization` object used for the process.
     */
    virtual void serialize(const Serialization& serialization) {}

    /**
     * @brief Serializes the object to the given `Json` object.
     *
     * @param dest The destination JSON object where the serialized data will be stored.
     */
    void serializeTo(Json& dest) {
        Serialization s;
        s.action = SerializationAction::Save;
        s.data   = JsonObject();
        serialize(s);
        dest = std::move(s.data);
    }

    /**
     * @brief Deserializes the object from the given `Json` object.
     *
     * @param src The source JSON object to load the data from.
     */
    void deserializeFrom(const Json& src) {
        Serialization s;
        s.action = SerializationAction::Load;
        s.data   = src;
        serialize(s);
    }

    /**
     * @brief Friend function to convert a `SerializableInterface` object to JSON.
     *
     * @param json The resulting JSON object.
     * @param self The serializable object.
     * @return True on success.
     */
    friend bool toJson(Json& json, const SerializableInterface& self) {
        const_cast<SerializableInterface&>(self).serializeTo(json);
        return true;
    }

    /**
     * @brief Friend function to convert a JSON object to a `SerializableInterface` object.
     *
     * @param json The source JSON object.
     * @param self The target serializable object.
     * @return True on success.
     */
    friend bool fromJson(const Json& json, SerializableInterface& self) {
        self.deserializeFrom(json);
        return true;
    }

    /// Default constructor.
    SerializableInterface() = default;
};

inline void Serialization::operator()(SerializableInterface& value, std::string_view name) const {
    value.serialize(key(name));
}

/**
 * @brief Helper macro to define empty JSON conversion functions for a class.
 *
 * This is useful for classes that do not need to store any meaningful data in JSON format.
 *
 * @param CLASS The class name for which the conversion functions are defined.
 */
#define JSON_CONVERSION_EMPTY(CLASS)                                                                         \
    friend bool toJson(Json& json, const CLASS& self) {                                                      \
        json = nullptr;                                                                                      \
        return true;                                                                                         \
    }                                                                                                        \
    friend bool fromJson(const Json& json, CLASS& self) {                                                    \
        return true;                                                                                         \
    }

/**
 * @brief Specialization of `JsonConverter` for `std::optional<T>`.
 *
 * Provides serialization and deserialization logic for `std::optional` types.
 *
 * @tparam T The type contained within the std::optional.
 */
template <typename T>
struct JsonConverter<std::optional<T>> {
    /**
     * @brief Converts an `optional` value to JSON.
     *
     * @param result The resulting JSON object.
     * @param srcValue The source optional value.
     * @return True on success.
     */
    static bool toJson(Json& result, const std::optional<T>& srcValue);

    /**
     * @brief Converts a JSON object to an `optional` value.
     *
     * @param srcJson The source JSON object.
     * @param result The target optional value.
     * @return True on success.
     */
    static bool fromJson(const Json& srcJson, std::optional<T>& result);
};

} // namespace Brisk
