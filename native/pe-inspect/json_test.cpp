#include "json.h"

#include <iostream>
#include <stdexcept>
#include <string>

using raven::inspect::JsonValue;
using raven::inspect::serialize_json;

static void check(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

int main()
{
	try {
		JsonValue::Object report;
		report.emplace_back("schema_version", "1");
		report.emplace_back("status", "ok");
		report.emplace_back("count", uint64_t{18446744073709551615ULL});
		report.emplace_back("negative", int64_t{-42});
		report.emplace_back("enabled", true);
		report.emplace_back("missing", nullptr);
		report.emplace_back("items", JsonValue::Array{JsonValue("first"), JsonValue("second")});
		const std::string expected =
			"{\"schema_version\":\"1\",\"status\":\"ok\",\"count\":18446744073709551615,"
			"\"negative\":-42,\"enabled\":true,\"missing\":null,\"items\":[\"first\",\"second\"]}";
		const auto serialized = serialize_json(JsonValue(std::move(report)));
		check(serialized == expected, "JSON serialization or insertion order differs");
		check(serialize_json(JsonValue(std::string("quote\" slash\\ line\n tab\t nul\0", 30))) ==
			"\"quote\\\" slash\\\\ line\\n tab\\t nul\\u0000\"", "JSON string escaping differs");
		check(serialize_json(JsonValue("UTF-8: café")) == "\"UTF-8: café\"", "UTF-8 bytes were not preserved");
		check(serialize_json(JsonValue(JsonValue::Array{})) == "[]", "empty array differs");
		check(serialize_json(JsonValue(JsonValue::Object{})) == "{}", "empty object differs");

		bool duplicate_rejected = false;
		try {
			(void)serialize_json(JsonValue(JsonValue::Object{{"same", 1}, {"same", 2}}));
		} catch (const std::invalid_argument &) {
			duplicate_rejected = true;
		}
		check(duplicate_rejected, "duplicate object keys were accepted");
		std::cout << "PE inspector JSON serialization contracts passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
