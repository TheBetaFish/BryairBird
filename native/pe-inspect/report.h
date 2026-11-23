#pragma once

#include "json.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace raven::inspect {

enum class ReportStatus { Ok, Partial, Error };
enum class DelayAddressMode { Rva, Va };

struct InputMetadata {
	std::string path;
	uint64_t size_bytes = 0;
	std::string sha256;
};

struct ImportSymbol {
	std::optional<std::string> name;
	std::optional<uint16_t> hint;
	std::optional<uint16_t> ordinal;
	std::optional<uint32_t> iat_rva;
};

struct ImportDll {
	std::string name;
	std::vector<ImportSymbol> symbols;
};

struct DelayImportDll {
	std::string name;
	uint32_t attributes = 0;
	DelayAddressMode address_mode = DelayAddressMode::Rva;
	std::vector<ImportSymbol> symbols;
};

struct Diagnostic {
	std::string code;
	std::string message;
	std::optional<JsonValue> location;
	// Stable, caller-provided ordering key derived from the input location.
	// Empty keys sort after located diagnostics; code breaks ties.
	std::string location_sort_key;
};

struct InspectorReport {
	std::string tool_version;
	InputMetadata input;
	ReportStatus status = ReportStatus::Error;
	// These are structured subtrees built by the PE-header and RT_VERSION adapters.
	// Null is valid when the corresponding structure is absent or unavailable.
	JsonValue pe;
	std::vector<ImportDll> imports;
	std::vector<DelayImportDll> delay_imports;
	JsonValue version_info;
	std::vector<Diagnostic> warnings;
	std::vector<Diagnostic> errors;
};

/// Build schema-v1 JSON in the normative top-level key order.
/// Throws std::invalid_argument when required metadata or symbol invariants are violated.
JsonValue report_json(const InspectorReport &report);

/// Serialize report_json() using the deterministic JSON serializer.
std::string serialize_report(const InspectorReport &report);

/// Lowercase ASCII DLL name normalization; non-ASCII UTF-8 bytes are preserved.
std::string normalize_dll_name(const std::string &name);

}
