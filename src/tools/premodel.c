// Minote - premodel.c
// External tool that converts Wavefront .obj to vertex data

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobjloader-c/tinyobj_loader_c.h"

#define  READALL_CHUNK  (256 * 1024)

#define  READALL_OK          0  /* Success */
#define  READALL_INVALID    -1  /* Invalid parameters */
#define  READALL_ERROR      -2  /* Stream error */
#define  READALL_TOOMUCH    -3  /* Too much input */
#define  READALL_NOMEM      -4  /* Out of memory */

/* This function returns one of the READALL_ constants above.
   If the return value is zero == READALL_OK, then:
     (*dataptr) points to a dynamically allocated buffer, with
     (*sizeptr) chars read from the file.
     The buffer is allocated for one extra char, which is NUL,
     and automatically appended after the data.
   Initial values of (*dataptr) and (*sizeptr) are ignored.
*/
int readall(FILE *in, char **dataptr, size_t *sizeptr)
{
	char *data = NULL, *temp;
	size_t size = 0;
	size_t used = 0;
	size_t n;

	/* None of the parameters can be NULL. */
	if (in == NULL || dataptr == NULL || sizeptr == NULL)
		return READALL_INVALID;

	/* A read error already occurred? */
	if (ferror(in))
		return READALL_ERROR;

	while (1) {

		if (used + READALL_CHUNK + 1 > size) {
			size = used + READALL_CHUNK + 1;

			/* Overflow check. Some ANSI C compilers
			   may optimize this away, though. */
			if (size <= used) {
				free(data);
				return READALL_TOOMUCH;
			}

			temp = realloc(data, size);
			if (temp == NULL) {
				free(data);
				return READALL_NOMEM;
			}
			data = temp;
		}

		n = fread(data + used, 1, READALL_CHUNK, in);
		if (n == 0)
			break;

		used += n;
	}

	if (ferror(in)) {
		free(data);
		return READALL_ERROR;
	}

	temp = realloc(data, used + 1);
	if (temp == NULL) {
		free(data);
		return READALL_NOMEM;
	}
	data = temp;
	data[used] = '\0';

	*dataptr = data;
	*sizeptr = used;

	return READALL_OK;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		puts("premodel - converts Wavefront .obj files to a C-style list of floats");
		puts("Usage: premodel inputFile outputFile");
		exit(0);
	}

	FILE *input = NULL;
	FILE *output = NULL;
	input = fopen(argv[1], "r");
	if (input == NULL) {
		fprintf(stderr, "Could not open %s for reading: %s\n",
		        argv[1], strerror(errno));
		exit(1);
	}
	output = fopen(argv[2], "w");
	if (output == NULL) {
		fprintf(stderr, "Could not open %s for writing: %s\n",
		        argv[2], strerror(errno));
		exit(1);
	}

	char *inputBuffer = NULL;
	size_t inputBufferChars = 0;
	if (readall(input, &inputBuffer, &inputBufferChars) != READALL_OK) {
		fprintf(stderr, "Could not read contents of %s\n", argv[1]);
		exit(1);
	}
	fclose(input);

	tinyobj_attrib_t attrib;
	tinyobj_shape_t *shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t *materials = NULL;
	size_t num_materials;
	if (tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
	                      &num_materials, inputBuffer, inputBufferChars,
	                      TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS) {
		fprintf(stderr, "Could not convert the input file\n");
		exit(1);
	}

	for (int i = 0; i < attrib.num_faces; i += 3) {
		for (int j = 0; j < 3; j++) {
			int vertexIndex = attrib.faces[i + j].v_idx;
			int normalIndex = attrib.faces[i + j].vn_idx;
			fprintf(output, "%ff, %ff, %ff, %ff, %ff, %ff",
			        attrib.vertices[vertexIndex * 3 + 0],
			        attrib.vertices[vertexIndex * 3 + 1],
			        attrib.vertices[vertexIndex * 3 + 2],
			        attrib.normals[normalIndex * 3 + 0],
			        attrib.normals[normalIndex * 3 + 1],
			        attrib.normals[normalIndex * 3 + 2]);
			if (i + 4 < attrib.num_faces || j + 1 < 3)
				fprintf(output, ",");
			fprintf(output, "\n");
		}
		if (i + 4 < attrib.num_faces)
			fprintf(output, "\n");
	}

	if (materials)
		tinyobj_materials_free(materials, num_materials);
	if (shapes)
		tinyobj_shapes_free(shapes, num_shapes);
	fclose(output);
}
