#include "cli.h"

#include <charconv>
#include <limits>

namespace raven::inspect {

namespace {

bool number(std::string_view text, uint64_t &value)
{
	if (text.empty())
		return false;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value, 10);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size() && value > 0;
}

bool take_value(int &index, int argc, const char *const argv[], std::string_view option, std::string &value,
	std::string &error)
{
	if (index + 1 >= argc || !argv[index + 1] || argv[index + 1][0] == '\0' || argv[index + 1][0] == '-') {
		error = "missing value for " + std::string(option);
		return false;
	}
	value = argv[++index];
	return true;
}

}

ParseResult parse_arguments(int argc, const char *const argv[])
{
	ParseResult result;
	if (argc <= 1) {
		result.error = "missing command";
		return result;
	}
	if (!argv[1]) {
		result.error = "missing command";
		return result;
	}
	if (std::string_view(argv[1]) == "--help" || std::string_view(argv[1]) == "-h") {
		result.help = true;
		return result;
	}
	const std::string_view command(argv[1]);
	if (command == "inspect")
		result.options.command = Command::InspectFile;
	else if (command == "inspect-dir")
		result.options.command = Command::InspectDirectory;
	else {
		result.error = "unknown command: " + std::string(command);
		return result;
	}
	if (argc < 3 || !argv[2] || argv[2][0] == '\0' || argv[2][0] == '-') {
		result.error = "missing input path";
		return result;
	}
	result.options.input = argv[2];
	bool seen_format = false;
	bool seen_output = false;
	bool seen_max_size = false;
	bool seen_include = false;
	bool seen_recursive = false;
	for (int i = 3; i < argc; ++i) {
		if (!argv[i]) {
			result.error = "null argument";
			return result;
		}
		const std::string_view option(argv[i]);
		if (option == "--format") {
			std::string value;
			if (seen_format) {
				result.error = "duplicate --format";
				return result;
			}
			seen_format = true;
			if (!take_value(i, argc, argv, option, value, result.error))
				return result;
			if (result.options.command == Command::InspectFile) {
				if (value == "json")
					result.options.format = Format::Json;
				else if (value == "text")
					result.options.format = Format::Text;
				else {
					result.error = "inspect format must be json or text";
					return result;
				}
			} else {
				if (value != "jsonl") {
					result.error = "inspect-dir format must be jsonl";
					return result;
				}
				result.options.format = Format::JsonLines;
			}
		} else if (option == "--output") {
			if (seen_output) {
				result.error = "duplicate --output";
				return result;
			}
			seen_output = true;
			if (!take_value(i, argc, argv, option, result.options.output, result.error))
				return result;
		} else if (option == "--max-size") {
			std::string value;
			uint64_t parsed = 0;
			if (seen_max_size) {
				result.error = "duplicate --max-size";
				return result;
			}
			seen_max_size = true;
			if (!take_value(i, argc, argv, option, value, result.error))
				return result;
			if (!number(value, parsed)) {
				result.error = "--max-size must be a positive base-10 integer";
				return result;
			}
			result.options.max_size = parsed;
		} else if (option == "--include") {
			if (result.options.command != Command::InspectDirectory) {
				result.error = "--include is only valid with inspect-dir";
				return result;
			}
			if (seen_include) {
				result.error = "duplicate --include";
				return result;
			}
			seen_include = true;
			if (!take_value(i, argc, argv, option, result.options.include, result.error))
				return result;
		} else if (option == "--recursive") {
			if (result.options.command != Command::InspectDirectory) {
				result.error = "--recursive is only valid with inspect-dir";
				return result;
			}
			if (seen_recursive) {
				result.error = "duplicate --recursive";
				return result;
			}
			seen_recursive = true;
			result.options.recursive = true;
		} else {
			result.error = "unknown option: " + std::string(option);
			return result;
		}
	}
	if (result.options.command == Command::InspectDirectory && seen_max_size) {
		result.error = "--max-size is only valid with inspect";
		return result;
	}
	if (result.options.command == Command::InspectDirectory && seen_format && result.options.format != Format::JsonLines) {
		result.error = "inspect-dir requires JSON Lines";
		return result;
	}
	return result;
}

std::string usage()
{
	return "Usage:\n"
		"  inspector inspect <path> [--format json|text] [--output <file>] [--max-size <bytes>]\n"
		"  inspector inspect-dir <dir> [--recursive] [--include <globs>] [--format jsonl] [--output <file>]\n";
}

}
