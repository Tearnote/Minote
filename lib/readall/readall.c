#include "readall.h"

#include <stdlib.h>
#include <stdio.h>

#define  READALL_CHUNK  (256 * 1024)

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
