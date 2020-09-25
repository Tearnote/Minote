/**
 * Basic dynamic array type
 * @file
 * Simple stretchable tightly packed array. If the underlying storage is full,
 * it is reallocated to 2x current size. All functions in this interface are
 * O(1) unless specified otherwise.
 */

#ifndef MINOTE_DARRAY_H
#define MINOTE_DARRAY_H

#include <stdint.h>

/**
 * Dynamic array type. You can obtain an instance with darrayCreate().
 * All fields are read-only.
 * The #data field can be freely accessed as an array of #count elements
 * of #capacity size.
 */
typedef struct darray {
	uint8_t* data; ///< Dynamically reallocated array for storing elements
	size_t elementSize; ///< in bytes
	int count; ///< Number of elements currently in #data
	int capacity; ///< Number of elements that can fit in #data without resizing
} darray;

/**
 * Create a new ::darray instance.
 * @param elementSize Size of each element in bytes
 * @return A newly created ::darray. Needs to be destroyed with darrayDestroy()
 */
darray* darrayCreate(size_t elementSize);

/**
 * Destroy a ::darray instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param d The ::darray object
 */
void darrayDestroy(darray* d);

/**
 * Create a new element at the end of a ::darray and return it. If the ::darray
 * has no space for another element, more space is allocated permanently. The
 * produced element might not be zero-initialized if the ::darray was cleared
 * before. The returned pointer can only be considered valid until another call
 * to darrayProduce().
 * @param d The ::darray object
 * @return Pointer to the newly created element
 */
void* darrayProduce(darray* d);

/**
 * Remove an element at a given index. Other elements are shifted to fill
 * the gap, which makes this an O(n) operation that should be avoided
 * for any large arrays.
 * @param d The ::darray object
 * @param index Index of the element to remove
 */
void darrayRemove(darray* d, size_t index);

/**
 * Remove an element at a given index, and put the last element in its place.
 * This removal is an O(1) operation, at the cost of changing the ordering
 * of remaining elements.
 * @param d The ::darray object
 * @param indexIndex of the element to remove
 */
void darrayRemoveSwap(darray* d, size_t index);

/**
 * Return a ::darray element from a given index.
 * @param d The ::darray object
 * @param index Index of the element to return. Must be smaller than the current
 * count of elements
 * @return Address of the element at the given index
 */
void* darrayGet(darray* d, size_t index);

/**
 * Clear a ::darray by setting the number of elements to zero. The allocated
 * storage is not resized.
 * @param d The ::darray object
 */
void darrayClear(darray* d);

#endif //MINOTE_DARRAY_H
