#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#include <unistd.h>
#include "crc32.hpp"
static constexpr bool VERBOSE_COMPILER = true;

std::string compile_command(const std::string& cc,
	const std::string& outfile, const std::string& codefile,
	const std::string& arguments)
{
	return cc + " -O2 -static -std=c11 " + arguments + " -x c -o " + outfile + " " + codefile;
}
std::string env_with_default(const char* var, const std::string& defval) {
	std::string value = defval;
	if (const char* envval = getenv(var); envval) value = std::string(envval);
	return value;
}

std::vector<uint8_t> load_file(const std::string& filename)
{
	size_t size = 0;
	FILE* f = fopen(filename.c_str(), "rb");
	if (f == NULL) throw std::runtime_error("Could not open file: " + filename);

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	std::vector<uint8_t> result(size);
	if (size != fread(result.data(), 1, size, f))
	{
		fclose(f);
		throw std::runtime_error("Error when reading from file: " + filename);
	}
	fclose(f);
	return result;
}

std::string build(const std::string& code, const std::string& compiler_args)
{
	// Create temporary filenames for code and binary
	char code_filename[64];
	strncpy(code_filename, "/tmp/builder-XXXXXX", sizeof(code_filename));
	// Open temporary code file with owner privs
	const int code_fd = mkstemp(code_filename);
	if (code_fd < 0) {
		throw std::runtime_error(
			"Unable to create temporary file for code: " + std::string(code_filename));
	}
	// Write code to temp code file
	const ssize_t code_len = write(code_fd, code.c_str(), code.size());
	if (code_len < (ssize_t) code.size()) {
		unlink(code_filename);
		throw std::runtime_error("Unable to write to temporary file");
	}
	// Compile code to binary file
	char bin_filename[256];
	const uint32_t checksum = crc32(code.c_str(), code.size());
	(void)snprintf(bin_filename, sizeof(bin_filename),
		"/tmp/binary-%08X", checksum);

	auto cc = env_with_default("cc", "gcc");
	auto command = compile_command(cc, bin_filename, code_filename, compiler_args);
	if constexpr (VERBOSE_COMPILER) {
		printf("Command: %s\n", command.c_str());
	}
	// Compile program
	FILE* f = popen(command.c_str(), "r");
	if (f == nullptr) {
		unlink(code_filename);
		throw std::runtime_error("Unable to compile code");
	}
	pclose(f);
	unlink(code_filename);

	return bin_filename;
}
std::vector<uint8_t> build_and_load(const std::string& code)
{
	return load_file(build(code, ""));
}
std::pair<
	std::string,
	std::vector<uint8_t>
> build_and_load(const std::string& code, const std::string& args)
{
	const auto file = build(code, args);
	return {file, load_file(file)};
}
