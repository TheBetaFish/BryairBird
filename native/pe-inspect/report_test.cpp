#include "report.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace raven::inspect;

static void check(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

int main()
{
	try {
		InspectorReport report;
		report.tool_version = "0.1.0";
		report.input = {"sample.exe", 123, std::string(64, 'a')};
		report.status = ReportStatus::Partial;
		report.imports = {
			{"KERNEL32.DLL", {ImportSymbol{std::string("CreateFileW"), uint16_t{4}, std::nullopt, uint32_t{0x1234}},
				ImportSymbol{std::nullopt, std::nullopt, uint16_t{17}, std::nullopt}}},
			{"api-ms-win-core-file-l1-1-0.dll", {}}
		};
		report.delay_imports = {
			{"kernel32.dll", 1, DelayAddressMode::Va, {}},
			{"USER32.dll", 0, DelayAddressMode::Rva, {}}
		};
		report.warnings = {
			{"PE_LATER", "later location", JsonValue(JsonValue::Object{{"rva", "0x20"}}), "rva:000020"},
			{"PE_FIRST", "first location", JsonValue(JsonValue::Object{{"rva", "0x10"}}), "rva:000010"},
			{"PE_UNLOCATED", "no location", std::nullopt, ""}
		};
		const auto output = serialize_report(report);
		const std::string expected_prefix =
			"{\"schema_version\":\"1\",\"tool\":{\"name\":\"bryairbird-pe-inspector\",\"version\":\"0.1.0\"},"
			"\"input\":{\"path\":\"sample.exe\",\"size_bytes\":123,\"sha256\":\"" + std::string(64, 'a') +
			"\"},\"status\":\"partial\",\"pe\":null,\"imports\":[";
		check(output.rfind(expected_prefix, 0) == 0, "schema-v1 top-level order or metadata differs");
		check(output.find("\"iat_rva\":\"0x1234\"") != std::string::npos, "IAT RVA was not formatted as lowercase hex");
		check(output.find("\"address_mode\":\"va\"") != std::string::npos, "delay address mode was not serialized");
		check(output.find("\"normalized_name\":\"kernel32.dll\"") != std::string::npos, "DLL name normalization differs");
		check(output.find("\"kind\":\"api_set\"") != std::string::npos, "API-set dependency classification differs");
		check(output.find("\"name\":\"KERNEL32.DLL\"") != std::string::npos, "original DLL spelling was not preserved");
		check(output.find("\"code\":\"PE_FIRST\"") < output.find("\"code\":\"PE_LATER\""), "diagnostics were not location-sorted");
		check(output.find("\"code\":\"PE_UNLOCATED\"") > output.find("\"code\":\"PE_LATER\""), "unlocated diagnostic was not last");
		check(output.find("\"normalized_name\":\"kernel32.dll\"") != output.rfind("\"normalized_name\":\"kernel32.dll\""),
			"imports and dependencies were not both represented");

		report.input.sha256 = "ABC";
		bool rejected_hash = false;
		try {
			(void)report_json(report);
		} catch (const std::invalid_argument &) {
			rejected_hash = true;
		}
		check(rejected_hash, "invalid SHA-256 was accepted");
		report.input.sha256 = std::string(64, 'b');
		report.imports[0].symbols[0].hint.reset();
		bool rejected_symbol = false;
		try {
			(void)report_json(report);
		} catch (const std::invalid_argument &) {
			rejected_symbol = true;
		}
		check(rejected_symbol, "named import without hint was accepted");
		std::cout << "PE inspector report model contracts passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
