// Minote - darray.c

#include "array.h"

#include <stdlib.h>
#include <stdbool.h>

#include "util.h"

struct darray *createDarray(size_t itemSize)
{
	struct darray *d = allocate(sizeof(*d));
	d->itemSize = itemSize;
	d->buffer = allocate(itemSize);
	d->allocated = 1;
	return d;
}

void destroyDarray(struct darray *d)
{
	free(d->buffer);
	free(d);
}

void *produceDarrayItem(struct darray *d)
{
	// Grow the buffer if there is no space left
	if (d->count == d->allocated) {
		size_t oldSize = d->allocated * d->itemSize;
		d->buffer = reallocate(d->buffer, oldSize * 2);
		memset(d->buffer + oldSize, 0, oldSize);
		d->allocated *= 2;
	}
	d->count += 1;
	// Simply return the first unused item and mark it as used
	return getDarrayItem(d, d->count - 1);
}

void *getDarrayItem(struct darray *d, int index)
{
	return d->buffer + index * d->itemSize;
}

void clearDarray(struct darray *d)
{
	d->count = 0;
}

bool isDarrayEmpty(darray *d)
{
	return d->count == 0;
}

// ========================================

struct vdarray *createVdarray(void)
{
	struct vdarray *vd = allocate(sizeof(*vd));
	vd->buffer = allocate(1);
	vd->allocated = 1;
	return vd;
}

void destroyVdarray(struct vdarray *vd)
{
	free(vd->buffer);
	free(vd);
}

void *produceVdarrayItem(struct vdarray *vd, size_t itemSize)
{
	while (vd->size + itemSize > vd->allocated) {
		vd->buffer = reallocate(vd->buffer, vd->allocated * 2);
		memset(vd->buffer + vd->allocated, 0, vd->allocated);
		vd->allocated *= 2;
	}

	void *result = vd->buffer + vd->size;
	vd->size += itemSize;
	return result;
}

void *getVdarrayItem(struct vdarray *vd, size_t offset)
{
	return vd->buffer + offset;
}

void clearVdarray(vdarray *vd)
{
	vd->size = 0;
}

bool isVdarrayEmpty(vdarray *vd)
{
	return vd->size == 0;
}

// ========================================

struct pdarray *createPdarray(size_t itemSize)
{
	struct pdarray *pd = allocate(sizeof(*pd));
	pd->itemSize = itemSize;
	pd->buffer = allocate(itemSize);
	pd->allocated = 1;
	pd->dead = createDarray(sizeof(bool));
	return pd;
}

void destroyPdarray(struct pdarray *pd)
{
	destroyDarray(pd->dead);
	free(pd->buffer);
	free(pd);
}

void *producePdarrayItem(struct pdarray *pd)
{
	// First, check for dead items
	for (int i = 0; i < pd->dead->count; i++) {
		bool *dead = getDarrayItem(pd->dead, i);
		if (!*dead)
			continue;
		*dead = false;
		void *item = getPdarrayItem(pd, i);
		memset(item, 0, pd->itemSize);
		return item;
	}

	// Grow the buffer if there is no space left
	if (pd->count == pd->allocated) {
		size_t oldSize = pd->allocated * pd->itemSize;
		pd->buffer = reallocate(pd->buffer, oldSize * 2);
		memset(pd->buffer + oldSize, 0, oldSize);
		pd->allocated *= 2;
	}
	pd->count += 1;
	bool *dead = produceDarrayItem(pd->dead);
	*dead = false;
	// Simply return the first unused item and mark it as used
	assert(pd->count == pd->dead->count);
	return getPdarrayItem(pd, pd->count - 1);
}

void killPdarrayItem(pdarray *pd, int index)
{
	bool *dead = getDarrayItem(pd->dead, index);
	*dead = true;
}

bool isPdarrayItemAlive(pdarray *pd, int index)
{
	bool *dead = getDarrayItem(pd->dead, index);
	return !*dead;
}

void *getPdarrayItem(struct pdarray *pd, int index)
{
	return pd->buffer + index * pd->itemSize;
}

void clearPdarray(struct pdarray *pd)
{
	pd->count = 0;
	clearDarray(pd->dead);
}

bool isPdarrayEmpty(struct pdarray *pd)
{
	return pd->count == 0;
}

// ========================================

struct psarray *createPsarray(size_t itemSize, int items)
{
	struct psarray *ps = allocate(sizeof(*ps));
	ps->itemSize = itemSize;
	ps->buffer = allocate(itemSize * items);
	ps->allocated = items;
	ps->dead = createDarray(sizeof(bool));
	return ps;
}

void destroyPsarray(struct psarray *ps)
{
	destroyDarray(ps->dead);
	free(ps->buffer);
	free(ps);
}

void *producePsarrayItem(struct psarray *ps)
{
	// First, check for dead items
	for (int i = 0; i < ps->dead->count; i++) {
		bool *dead = getDarrayItem(ps->dead, i);
		if (!*dead)
			continue;
		*dead = false;
		void *item = getPsarrayItem(ps, i);
		memset(item, 0, ps->itemSize);
		return item;
	}

	// Return a new item
	if (ps->count == ps->allocated)
		return NULL;
	ps->count += 1;
	bool *dead = produceDarrayItem(ps->dead);
	*dead = false;
	// Simply return the first unused item and mark it as used
	assert(ps->count == ps->dead->count);
	return getPsarrayItem(ps, ps->count - 1);
}

void killPsarrayItem(psarray *ps, int index)
{
	bool *dead = getDarrayItem(ps->dead, index);
	*dead = true;
}

bool isPsarrayItemAlive(psarray *ps, int index)
{
	bool *dead = getDarrayItem(ps->dead, index);
	return !*dead;
}

void *getPsarrayItem(struct psarray *ps, int index)
{
	return ps->buffer + index * ps->itemSize;
}

void clearPsarray(struct psarray *ps)
{
	ps->count = 0;
	clearDarray(ps->dead);
}

bool isPsarrayEmpty(struct psarray *ps)
{
	return ps->count == 0;
}