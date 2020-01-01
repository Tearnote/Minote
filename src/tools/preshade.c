/**
 * External tool that preprocesses shaders so that they can be included in
 * the source
 * @file
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	if (argc != 3) {
		puts("preshade - preprocesses shaders so that they can be included in the source");
		puts("Usage: preshade inputFile outputFile");
		exit(EXIT_SUCCESS);
	}

	FILE* input = NULL;
	FILE* output = NULL;

	input = fopen(argv[1], "r");
	if (!input) {
		fprintf(stderr, "Could not open %s for reading: %s\n",
				argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	output = fopen(argv[2], "w");
	if (!output) {
		fprintf(stderr, "Could not open %s for writing: %s\n",
				argv[2], strerror(errno));
		exit(EXIT_FAILURE);
	}

	int ch = 0;
	bool first = true;
	int chCount = 0;
	while ((ch = fgetc(input)) != EOF) {
		// Print comma if needed
		if (first)
			first = false;
		else
			fprintf(output, ", ");

		// Go to a new line after 8 chars
		if (chCount == 8) {
			fprintf(output, "\n");
			chCount = 0;
		}
		chCount += 1;

		// Do the thing
		fprintf(output, "0x%02x", ch);
	}

	fclose(output);
	fclose(input);
}