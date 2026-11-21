#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace raven::inspect {

struct JsonValue {
	using Array = std::vector<JsonValue>;
	using Object = std::vector<std::pair<std::string, JsonValue>>;
	using Storage = std::variant<std::nullptr_t, bool, int64_t, uint64_t, std::string, Array, Object>;

	Storage value;

	JsonValue() : value(nullptr) {}
	JsonValue(std::nullptr_t) : value(nullptr) {}
	JsonValue(bool item) : value(item) {}
	JsonValue(int64_t item) : value(item) {}
	JsonValue(uint64_t item) : value(item) {}
	JsonValue(std::string item) : value(std::move(item)) {}
	JsonValue(const char *item) : value(std::string(item ? item : "")) {}
	JsonValue(Array item) : value(std::move(item)) {}
	JsonValue(Object item) : value(std::move(item)) {}
};

/// Serialize compact JSON with stable array/object insertion order and no insignificant whitespace.
/// Object keys must be unique. Strings are emitted as UTF-8 with JSON-required escaping.
std::string serialize_json(const JsonValue &value);

}
