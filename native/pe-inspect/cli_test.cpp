#include "cli.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using raven::inspect::Command;
using raven::inspect::Format;
using raven::inspect::parse_arguments;

static void check(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static auto parse(std::vector<const char *> args)
{
	return parse_arguments(static_cast<int>(args.size()), args.data());
}

int main()
{
	try {
		auto file = parse({"inspector", "inspect", "client.exe"});
		check(file && file.options.command == Command::InspectFile && file.options.format == Format::Json,
			"default inspect arguments differ");
		check(file.options.max_size == 512ULL * 1024 * 1024, "default size limit differs");

		auto text = parse({"inspector", "inspect", "client.exe", "--format", "text", "--max-size", "4096"});
		check(text && text.options.format == Format::Text && text.options.max_size == 4096, "file options were not parsed");

		auto directory_default = parse({"inspector", "inspect-dir", "packages"});
		check(directory_default && directory_default.options.format == Format::JsonLines,
			"directory default format is not JSON Lines");

		auto directory = parse({"inspector", "inspect-dir", "packages", "--recursive", "--include", "*.dll,*.exe", "--format", "jsonl"});
		check(directory && directory.options.command == Command::InspectDirectory && directory.options.recursive &&
			directory.options.format == Format::JsonLines && directory.options.include == "*.dll,*.exe",
			"directory options were not parsed");

		for (const auto &args : std::vector<std::vector<const char *>>{
			{"inspector"}, {"inspector", "unknown", "x"}, {"inspector", "inspect"},
			{"inspector", "inspect", "x", "--format", "jsonl"}, {"inspector", "inspect", "x", "--max-size", "0"},
			{"inspector", "inspect", "x", "--max-size", "-1"}, {"inspector", "inspect", "x", "--max-size", "1x"},
			{"inspector", "inspect", "x", "--format"}, {"inspector", "inspect", "x", "--output"},
			{"inspector", "inspect", "x", "--include", "*.dll"}, {"inspector", "inspect", "x", "--recursive"},
			{"inspector", "inspect-dir", "x", "--format", "text"}, {"inspector", "inspect-dir", "x", "--max-size", "10"},
			{"inspector", "inspect", "x", "--unknown"}, {"inspector", "inspect", "x", "--format", "json", "--format", "text"},
			{"inspector", "inspect-dir", "x", "--recursive", "--recursive"}}) {
			check(!parse(args), "invalid argument list was accepted");
		}
		auto help = parse({"inspector", "--help"});
		check(help && help.help, "help option was not recognized");
		check(!raven::inspect::usage().empty(), "usage text is empty");
		std::cout << "PE inspector CLI argument contracts passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
