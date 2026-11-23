#include "report.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace raven::inspect {

namespace {

const char *status_name(ReportStatus status)
{
	switch (status) {
	case ReportStatus::Ok: return "ok";
	case ReportStatus::Partial: return "partial";
	case ReportStatus::Error: return "error";
	}
	throw std::invalid_argument("invalid report status");
}

const char *address_mode_name(DelayAddressMode mode)
{
	switch (mode) {
	case DelayAddressMode::Rva: return "rva";
	case DelayAddressMode::Va: return "va";
	}
	throw std::invalid_argument("invalid delay import address mode");
}

std::string hex_value(uint64_t value)
{
	static constexpr char digits[] = "0123456789abcdef";
	char buffer[18] = "0x";
	std::size_t cursor = sizeof(buffer);
	do {
		buffer[--cursor] = digits[value & 0xf];
		value >>= 4;
	} while (value);
	return "0x" + std::string(buffer + cursor, sizeof(buffer) - cursor);
}

bool is_null(const JsonValue &value)
{
	return std::holds_alternative<std::nullptr_t>(value.value);
}

bool is_object(const JsonValue &value)
{
	return std::holds_alternative<JsonValue::Object>(value.value);
}

JsonValue symbol_json(const ImportSymbol &symbol)
{
	const bool named = symbol.name.has_value();
	const bool ordinal = symbol.ordinal.has_value();
	if (named == ordinal)
		throw std::invalid_argument("import symbol must have exactly one of name or ordinal");
	if (named != symbol.hint.has_value())
		throw std::invalid_argument("named import symbol must include a hint; ordinal symbol must not");
	JsonValue::Object result;
	if (named) {
		result.emplace_back("name", *symbol.name);
		result.emplace_back("hint", static_cast<uint64_t>(*symbol.hint));
	} else {
		result.emplace_back("ordinal", static_cast<uint64_t>(*symbol.ordinal));
	}
	if (symbol.iat_rva)
		result.emplace_back("iat_rva", hex_value(*symbol.iat_rva));
	return JsonValue(std::move(result));
}

JsonValue import_json(const ImportDll &dll)
{
	if (dll.name.empty())
		throw std::invalid_argument("import DLL name must not be empty");
	JsonValue::Array symbols;
	symbols.reserve(dll.symbols.size());
	for (const auto &symbol : dll.symbols)
		symbols.emplace_back(symbol_json(symbol));
	return JsonValue(JsonValue::Object{
		{"name", dll.name},
		{"normalized_name", normalize_dll_name(dll.name)},
		{"symbols", JsonValue(std::move(symbols))}
	});
}

JsonValue delay_import_json(const DelayImportDll &dll)
{
	if (dll.name.empty())
		throw std::invalid_argument("delay import DLL name must not be empty");
	JsonValue::Array symbols;
	symbols.reserve(dll.symbols.size());
	for (const auto &symbol : dll.symbols)
		symbols.emplace_back(symbol_json(symbol));
	return JsonValue(JsonValue::Object{
		{"name", dll.name},
		{"normalized_name", normalize_dll_name(dll.name)},
		{"attributes", static_cast<uint64_t>(dll.attributes)},
		{"address_mode", address_mode_name(dll.address_mode)},
		{"symbols", JsonValue(std::move(symbols))}
	});
}

struct Dependency {
	std::string name;
	std::string normalized_name;
	std::string kind;
};

std::vector<Dependency> dependencies(const InspectorReport &report)
{
	std::vector<Dependency> result;
	const auto add = [&result](const std::string &name) {
		if (name.empty())
			throw std::invalid_argument("dependency DLL name must not be empty");
		const auto normalized = normalize_dll_name(name);
		const auto found = std::find_if(result.begin(), result.end(), [&normalized](const Dependency &item) {
			return item.normalized_name == normalized;
		});
		if (found != result.end())
			return;
		const bool api_set = normalized.rfind("api-ms-win-", 0) == 0 || normalized.rfind("ext-ms-win-", 0) == 0;
		result.push_back({name, normalized, api_set ? "api_set" : "named_dll"});
	};
	for (const auto &dll : report.imports)
		add(dll.name);
	for (const auto &dll : report.delay_imports)
		add(dll.name);
	std::sort(result.begin(), result.end(), [](const Dependency &left, const Dependency &right) {
		return std::tie(left.normalized_name, left.name) < std::tie(right.normalized_name, right.name);
	});
	return result;
}

JsonValue diagnostic_array(const std::vector<Diagnostic> &diagnostics)
{
	std::vector<const Diagnostic *> ordered;
	ordered.reserve(diagnostics.size());
	for (const auto &diagnostic : diagnostics) {
		if (diagnostic.code.empty() || diagnostic.message.empty())
			throw std::invalid_argument("diagnostic code and message must not be empty");
		ordered.push_back(&diagnostic);
	}
	std::stable_sort(ordered.begin(), ordered.end(), [](const Diagnostic *left, const Diagnostic *right) {
		const bool left_located = left->location.has_value();
		const bool right_located = right->location.has_value();
		if (left_located != right_located)
			return left_located;
		if (left->location_sort_key != right->location_sort_key)
			return left->location_sort_key < right->location_sort_key;
		return left->code < right->code;
	});
	JsonValue::Array result;
	result.reserve(ordered.size());
	for (const auto *diagnostic : ordered) {
		JsonValue::Object item{
			{"code", diagnostic->code},
			{"message", diagnostic->message}
		};
		if (diagnostic->location)
			item.emplace_back("location", *diagnostic->location);
		result.emplace_back(std::move(item));
	}
	return JsonValue(std::move(result));
}

}

std::string normalize_dll_name(const std::string &name)
{
	std::string normalized = name;
	for (char &character : normalized) {
		if (character >= 'A' && character <= 'Z')
			character = static_cast<char>(character - 'A' + 'a');
	}
	return normalized;
}

JsonValue report_json(const InspectorReport &report)
{
	if (report.tool_version.empty())
		throw std::invalid_argument("tool version must not be empty");
	if (report.input.sha256.size() != 64 ||
		!std::all_of(report.input.sha256.begin(), report.input.sha256.end(), [](char value) {
			return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f');
		}))
		throw std::invalid_argument("input SHA-256 must be 64 lowercase hexadecimal characters");
	if (!is_null(report.pe) && !is_object(report.pe))
		throw std::invalid_argument("PE report subtree must be an object or null");
	if (!is_null(report.version_info) && !is_object(report.version_info))
		throw std::invalid_argument("version information subtree must be an object or null");

	JsonValue::Array imports;
	imports.reserve(report.imports.size());
	for (const auto &dll : report.imports)
		imports.emplace_back(import_json(dll));

	JsonValue::Array delay_imports;
	delay_imports.reserve(report.delay_imports.size());
	for (const auto &dll : report.delay_imports)
		delay_imports.emplace_back(delay_import_json(dll));

	JsonValue::Array dependency_items;
	const auto found_dependencies = dependencies(report);
	dependency_items.reserve(found_dependencies.size());
	for (const auto &dependency : found_dependencies) {
		dependency_items.emplace_back(JsonValue::Object{
			{"name", dependency.name},
			{"normalized_name", dependency.normalized_name},
			{"kind", dependency.kind}
		});
	}

	JsonValue::Object input{
		{"path", report.input.path},
		{"size_bytes", report.input.size_bytes},
		{"sha256", report.input.sha256}
	};
	JsonValue::Object tool{
		{"name", "bryairbird-pe-inspector"},
		{"version", report.tool_version}
	};
	return JsonValue(JsonValue::Object{
		{"schema_version", "1"},
		{"tool", JsonValue(std::move(tool))},
		{"input", JsonValue(std::move(input))},
		{"status", status_name(report.status)},
		{"pe", report.pe},
		{"imports", JsonValue(std::move(imports))},
		{"delay_imports", JsonValue(std::move(delay_imports))},
		{"version_info", report.version_info},
		{"dependencies", JsonValue(std::move(dependency_items))},
		{"warnings", diagnostic_array(report.warnings)},
		{"errors", diagnostic_array(report.errors)}
	});
}

std::string serialize_report(const InspectorReport &report)
{
	return serialize_json(report_json(report));
}

}
