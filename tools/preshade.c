// Minote - preshade.c
// External tool that preprocesses shaders so that they can be included in the source

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
	if(argc != 3) {
		puts("preshade - preprocesses shaders so that they can be included in the source");
		puts("Usage: preshade inputfile outputfile");
		exit(0);
	}
	
	FILE* input = NULL;
	FILE* output = NULL;
	
	input = fopen(argv[1], "r");
	if(input == NULL) {
		fprintf(stderr, "Could not open %s for reading: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	output = fopen(argv[2], "w");
	if(output == NULL) {
		fprintf(stderr, "Could not open %s for writing: %s\n", argv[2], strerror(errno));
		exit(1);
	}
	
	int ch = 0;
	bool first = true;
	int chcount = 0;
	while((ch = fgetc(input)) != EOF) {
		// Print comma if needed
		if(first)
			first = false;
		else
			fprintf(output, ", ");
		
		// Go to a new line after 8 chars
		if(chcount == 8) {
			fprintf(output, "\n");
			chcount = 0;
		}
		chcount += 1;
		
		// Do the thing
		fprintf(output, "0x%02x", ch);
	}
	
	fclose(output);
	fclose(input);
}