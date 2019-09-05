// Minote - premodel.c
// External tool that converts Wavefront .obj to vertex data

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobjloader-c/tinyobj_loader_c.h"

#include "readall/readall.h"

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
