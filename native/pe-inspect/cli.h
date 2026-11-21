#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace raven::inspect {

enum class Command { InspectFile, InspectDirectory };
enum class Format { Json, Text, JsonLines };

struct Options {
	Command command = Command::InspectFile;
	Format format = Format::Json;
	std::string input;
	std::string output;
	std::string include = "*.dll,*.exe";
	uint64_t max_size = 512ULL * 1024 * 1024;
	bool recursive = false;
};

struct ParseResult {
	Options options;
	std::string error;
	bool help = false;

	explicit operator bool() const { return error.empty(); }
};

ParseResult parse_arguments(int argc, const char *const argv[]);
std::string usage();

}
