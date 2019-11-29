// Minote - types/array.h
// Data structures with variable length
// darray  - Dynamic array. Allocates more memory each time the limit is reached
// vdarray - Variable member size dynamic array. darray where each member can
//           have different size. Access is by byte offset rather than by index
// pdarray - Pooled dynamic array. Members can be declared dead, requesting
//           a new member will return a dead member if possible
// psarray - Pooled static array. pdarray with fixed maximum size, pointers
//           to the data are guaranteed to stay valid

#ifndef TYPES_ARRAY_H
#define TYPES_ARRAY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct darray {
	uint8_t *buffer;
	size_t itemSize;
	int count; // Number of items present
	int allocated; // Number of items that can fit in the buffer
} darray;

darray *createDarray(size_t itemSize);
void destroyDarray(darray *d);
void *produceDarrayItem(darray *d);
void *getDarrayItem(darray *d, int index);
void clearDarray(darray *d);
bool isDarrayEmpty(darray *d);

typedef struct vdarray {
	uint8_t *buffer;
	int size; // in bytes
	int allocated; // in bytes
} vdarray;

vdarray *createVdarray(void);
void destroyVdarray(vdarray *vd);
void *produceVdarrayItem(vdarray *vd, size_t itemSize);
void *getVdarrayItem(vdarray *vd, size_t offset);
void clearVdarray(vdarray *vd);
bool isVdarrayEmpty(vdarray *vd);

typedef struct pdarray {
	uint8_t *buffer;
	darray *dead;
	size_t itemSize;
	int count; // Number of items present
	int allocated; // Number of items that can fit in the buffer
} pdarray;

pdarray *createPdarray(size_t itemSize);
void destroyPdarray(pdarray *pd);
void *producePdarrayItem(pdarray *pd);
void killPdarrayItem(pdarray *pd, int index);
bool isPdarrayItemAlive(pdarray *pd, int index);
void *getPdarrayItem(pdarray *pd, int index);
void clearPdarray(pdarray *pd);
bool isPdarrayEmpty(pdarray *pd);

typedef struct psarray {
	uint8_t *buffer;
	darray *dead;
	size_t itemSize;
	int count; // Number of items present
	int allocated; // Number of items that can fit in the buffer
} psarray;

psarray *createPsarray(size_t itemSize, int items);
void destroyPsarray(psarray *ps);
void *producePsarrayItem(psarray *ps);
void killPsarrayItem(psarray *ps, int index);
bool isPsarrayItemAlive(psarray *ps, int index);
void *getPsarrayItem(psarray *ps, int index);
void clearPsarray(psarray *ps);
bool isPsarrayEmpty(psarray *ps);

#endif // TYPES_ARRAY_H
