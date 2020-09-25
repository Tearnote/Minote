/**
 * External tool that preprocesses shaders so that they can be included in
 * the source
 * @file
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cctype>
#include "../util.hpp"

static char dir[256]{'\0'};

// https://stackoverflow.com/a/122974
char* trim(char* str)
{
	if (str == nullptr) { return nullptr; }
	if (str[0] == '\0') { return str; }

	size_t len{std::strlen(str)};
	char* frontp{str};
	char* endp{str + len};

	/* Move the front and back pointers to address the first non-whitespace
	 * characters from each end. */
	while (std::isspace((unsigned char)*frontp)) { ++frontp; }
	if (endp != frontp)
		while (std::isspace((unsigned char)*(--endp)) && endp != frontp) {}

	if (frontp != str && endp == frontp)
		*str = '\0';
	else if (str + len - 1 != endp)
		*(endp + 1) = '\0';

	/* Shift the string so that it starts at str so that if it's dynamically
	 * allocated, we can still free it on the returned pointer.  Note the reuse
	 * of endp to mean the front of the string buffer now.
	 */
	endp = str;
	if (frontp != str) {
		while (*frontp) { *endp++ = *frontp++; }
		*endp = '\0';
	}

	return str;
}

void fileProcess(const char* const filename, FILE* output)
{
	std::FILE* input{std::fopen(filename, "r")};
	if (!input) {
		std::fprintf(stderr, "Could not open %s for reading: %s\n",
			filename, std::strerror(errno));
		std::exit(EXIT_FAILURE);
	}

	char line[256]{'\0'};
	while (std::fgets(line, 256, input)) {
		if (line[std::strlen(line) - 1] != '\n') {
			std::fprintf(stderr, "Line longer than 256 chars, aborting\n");
			std::exit(EXIT_FAILURE);
		}
		trim(line);
		if (std::strncmp(line, "#include", std::strlen("#include")) == 0 &&
			!std::isalnum(line[std::strlen("#include")]) &&
			line[std::strlen("#include")] != '_') {
			// Is an #include directive
			char includeFile[256]{'\0'};
			if (!std::sscanf(line, "#include \"%s", includeFile)) {
				std::fprintf(stderr, "Syntax error in #include line: %s", line);
				std::exit(EXIT_FAILURE);
			}
			if (includeFile[std::strlen(includeFile) - 1] != '"') {
				std::fprintf(stderr, "Syntax error in #include line: %s", line);
				std::exit(EXIT_FAILURE);
			}
			includeFile[std::strlen(includeFile) - 1] = '\0';

			char includePath[256]{'\0'};
			std::sprintf(includePath, "%s/%s", dir, includeFile);

			fileProcess(includePath, output);
		} else {
			// Is a normal line
			std::size_t chCount = 0;
			for (std::size_t i = 0; i < std::strlen(line); i += 1) {
				// Go to a new line after 8 chars
				if (chCount == 8) {
					std::fprintf(output, "\n");
					chCount = 0;
				}
				chCount += 1;

				// Do the thing
				std::fprintf(output, "0x%02hhx, ", static_cast<unsigned char>(line[i]));
			}
			std::fprintf(output, "0x%02x,\n", '\n');
		}
	}

	std::fclose(input);
}

void dirname(char* path, char* dst)
{
	char* slash{std::strrchr(path, '/')};
	if (!slash) return;
	ptrdiff_t slashIndex{slash - path};
	std::strncpy(dst, path, slashIndex);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::puts("preshade - preprocesses shaders so that they can be included in the source");
		std::puts("Usage: preshade inputFile outputFile");
		std::exit(EXIT_SUCCESS);
	}

	FILE* output{std::fopen(argv[2], "w")};
	if (!output) {
		std::fprintf(stderr, "Could not open %s for writing: %s\n",
			argv[2], std::strerror(errno));
		std::exit(EXIT_FAILURE);
	}

	dirname(argv[1], dir);

	fileProcess(argv[1], output);

	std::fclose(output);
}
