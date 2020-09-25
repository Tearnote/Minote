/**
 * External tool that preprocesses shaders so that they can be included in
 * the source
 * @file
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "../util.hpp"

static char dir[256] = {'\0'};

// https://stackoverflow.com/a/122974
char *trim(char *str)
{
	size_t len = 0;
	char *frontp = str;
	char *endp = NULL;

	if( str == NULL ) { return NULL; }
	if( str[0] == '\0' ) { return str; }

	len = strlen(str);
	endp = str + len;

	/* Move the front and back pointers to address the first non-whitespace
	 * characters from each end.
	 */
	while( isspace((unsigned char) *frontp) ) { ++frontp; }
	if( endp != frontp )
	{
		while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
	}

	if( frontp != str && endp == frontp )
		*str = '\0';
	else if( str + len - 1 != endp )
		*(endp + 1) = '\0';

	/* Shift the string so that it starts at str so that if it's dynamically
	 * allocated, we can still free it on the returned pointer.  Note the reuse
	 * of endp to mean the front of the string buffer now.
	 */
	endp = str;
	if( frontp != str )
	{
		while( *frontp ) { *endp++ = *frontp++; }
		*endp = '\0';
	}

	return str;
}

void fileProcess(char* filename, FILE* output)
{
	FILE* input = null;
	input = fopen(filename, "r");
	if (!input) {
		fprintf(stderr, "Could not open %s for reading: %s\n",
			filename, strerror(errno));
		exit(EXIT_FAILURE);
	}

	char line[256] = {'\0'};
	while (fgets(line, 256, input)) {
		if (line[strlen(line) - 1] != '\n') {
			fprintf(stderr, "Line longer than 256 chars, aborting\n");
			exit(EXIT_FAILURE);
		}
		trim(line);
		if (strncmp(line, "#include", strlen("#include")) == 0 &&
		    !isalnum(line[strlen("#include")]) &&
			line[strlen("#include")] != '_') {
			// Is an #include directive
			char includeFile[256] = {'\0'};
			if (!sscanf(line, "#include \"%s", includeFile)) {
				fprintf(stderr, "Syntax error in #include line: %s", line);
				exit(EXIT_FAILURE);
			}
			if (includeFile[strlen(includeFile) - 1] != '"') {
				fprintf(stderr, "Syntax error in #include line: %s", line);
				exit(EXIT_FAILURE);
			}
			includeFile[strlen(includeFile) - 1] = '\0';

			char includePath[256] = {'\0'};
			sprintf(includePath, "%s/%s", dir, includeFile);

			fileProcess(includePath, output);
		} else {
			// Is a normal line
			size_t chCount = 0;
			for (size_t i = 0; i < strlen(line); i += 1) {
				// Go to a new line after 8 chars
				if (chCount == 8) {
					fprintf(output, "\n");
					chCount = 0;
				}
				chCount += 1;

				// Do the thing
				fprintf(output, "0x%02x, ", (unsigned char)line[i]);
			}
			fprintf(output, "0x%02x,\n", '\n');
		}
	}

	fclose(input);
}

void dirname(char* path, char* dst)
{
	char* slash = strrchr(path, '/');
	if (!slash) return;
	ptrdiff_t slashIndex = slash - path;
	strncpy(dst, path, slashIndex);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		puts("preshade - preprocesses shaders so that they can be included in the source");
		puts("Usage: preshade inputFile outputFile");
		exit(EXIT_SUCCESS);
	}

	FILE* output = null;
	output = fopen(argv[2], "w");
	if (!output) {
		fprintf(stderr, "Could not open %s for writing: %s\n",
				argv[2], strerror(errno));
		exit(EXIT_FAILURE);
	}

	dirname(argv[1], dir);

	fileProcess(argv[1], output);

	fclose(output);
}
