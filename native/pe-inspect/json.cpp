#include "json.h"

#include <charconv>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace raven::inspect {

namespace {

void append_string(std::string &out, std::string_view value)
{
	static constexpr char hex[] = "0123456789abcdef";
	out.push_back('"');
	for (const unsigned char byte : value) {
		switch (byte) {
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (byte < 0x20) {
				out += "\\u00";
				out.push_back(hex[byte >> 4]);
				out.push_back(hex[byte & 0x0f]);
			} else {
				out.push_back(static_cast<char>(byte));
			}
		}
	}
	out.push_back('"');
}

template<typename Integer>
void append_integer(std::string &out, Integer value)
{
	char buffer[std::numeric_limits<Integer>::digits10 + 4];
	const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
	if (result.ec != std::errc{})
		throw std::runtime_error("JSON integer formatting failed");
	out.append(buffer, result.ptr);
}

void append_value(std::string &out, const JsonValue &value)
{
	std::visit([&out](const auto &item) {
		using T = std::decay_t<decltype(item)>;
		if constexpr (std::is_same_v<T, std::nullptr_t>) {
			out += "null";
		} else if constexpr (std::is_same_v<T, bool>) {
			out += item ? "true" : "false";
		} else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
			append_integer(out, item);
		} else if constexpr (std::is_same_v<T, std::string>) {
			append_string(out, item);
		} else if constexpr (std::is_same_v<T, JsonValue::Array>) {
			out.push_back('[');
			bool first = true;
			for (const auto &element : item) {
				if (!first)
					out.push_back(',');
				first = false;
				append_value(out, element);
			}
			out.push_back(']');
		} else if constexpr (std::is_same_v<T, JsonValue::Object>) {
			out.push_back('{');
			bool first = true;
			for (size_t index = 0; index < item.size(); ++index) {
				for (size_t prior = 0; prior < index; ++prior) {
					if (item[prior].first == item[index].first)
						throw std::invalid_argument("duplicate JSON object key: " + item[index].first);
				}
				if (!first)
					out.push_back(',');
				first = false;
				append_string(out, item[index].first);
				out.push_back(':');
				append_value(out, item[index].second);
			}
			out.push_back('}');
		}
	}, value.value);
}

}

std::string serialize_json(const JsonValue &value)
{
	std::string result;
	append_value(result, value);
	return result;
}

}
