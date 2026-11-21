#include "json.h"

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace raven::inspect {

namespace {

void append_byte_escape(std::string &out, unsigned char byte)
{
	static constexpr char hex[] = "0123456789abcdef";
	out += "\\u00";
	out.push_back(hex[byte >> 4]);
	out.push_back(hex[byte & 0x0f]);
}

void append_string(std::string &out, std::string_view value)
{
	static constexpr char hex[] = "0123456789abcdef";
	out.push_back('"');
	for (size_t index = 0; index < value.size();) {
		const auto byte = static_cast<unsigned char>(value[index]);
		if (byte < 0x80) {
			switch (byte) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\b': out += "\\b"; break;
			case '\f': out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				if (byte < 0x20)
					append_byte_escape(out, byte);
				else
					out.push_back(static_cast<char>(byte));
			}
			++index;
			continue;
		}

		size_t length = 0;
		uint32_t codepoint = 0;
		if (byte >= 0xc2 && byte <= 0xdf) {
			length = 2;
			codepoint = byte & 0x1f;
		} else if (byte >= 0xe0 && byte <= 0xef) {
			length = 3;
			codepoint = byte & 0x0f;
		} else if (byte >= 0xf0 && byte <= 0xf4) {
			length = 4;
			codepoint = byte & 0x07;
		}
		bool valid = length != 0 && index + length <= value.size();
		if (valid) {
			for (size_t offset = 1; offset < length; ++offset) {
				const auto continuation = static_cast<unsigned char>(value[index + offset]);
				if ((continuation & 0xc0) != 0x80) {
					valid = false;
					break;
				}
				codepoint = (codepoint << 6) | (continuation & 0x3f);
			}
			valid = valid && !(length == 3 && codepoint >= 0xd800 && codepoint <= 0xdfff) &&
				!(length == 3 && codepoint < 0x800) && !(length == 4 && codepoint < 0x10000) &&
				codepoint <= 0x10ffff;
		}
		if (valid) {
			out.append(value.data() + index, length);
			index += length;
		} else {
			append_byte_escape(out, byte);
			++index;
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
