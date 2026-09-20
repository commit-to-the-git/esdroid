#ifndef YDS_MEMORY_BASE_H
#define YDS_MEMORY_BASE_H

#include "yds_base.h"

#include <malloc.h>
#include <new>
#include <stddef.h>
#include <assert.h>

static const int KB = 1024;
static const int MB = KB * KB;
static const int GB = KB * MB;

//
// standard base class for all memory allocators
// includes the standard interface for allocating
// generic and specific blocks
//
class ysMemoryAllocator : public ysObject
{

public:

    ysMemoryAllocator();
    ysMemoryAllocator(const char *typeID);
    ~ysMemoryAllocator();

    /* main interface */

    //
    // allocate a block of memory with a specific type
    //
    // n number of objects to allocate ie array length
    //
    // returns the allocated array
    //
    template<typename TYPE>
    TYPE *Allocate(unsigned int n=1)
    {

        // allocate an empty general block of memory
        void *block = AllocateBlock(sizeof(TYPE) * n, n);
        if (!block) return 0;

        // allocate each of the elements in the array
        for(unsigned int i=0; i < n; i++) 
            new((char *)block + i * sizeof(TYPE)) TYPE;

        return (TYPE *)block;

    }

    //
    // free a block of memory with a specific type
    //
    // data reference to a pointer the pointer is set to null after the operation
    //
    //
    template<typename TYPE>
    void Free(TYPE * &data)
    {

        int nObjects = FreeBlock((void *)data);
        for(int i=0; i < nObjects; i++) data[i].~TYPE();

        data = 0;

    }

    /* low level interface */

    //
    // allocate an arbitrary block of memory
    //
    // size total size bytes to allocate
    // numobjects expected number of objects to fill the block in the case of allocating an array
    //
    // returns a pointer to the new memory block null for failure
    //
    virtual void *AllocateBlock(int size, int numObjects=1) = 0;

    //
    // free a block of memory
    //
    // block pointer to target memory address
    //
    // returns the number of objects in the freed memory block
    //
    virtual int FreeBlock(void *block) = 0;

    //
    // destroy all allocated memory used by the allocator
    //
    // note this should be called before the allocators destructor
    // also any objects allocated within the address space of the allocator
    // left after a destroy call will not have their destructors called
    //
    virtual void Destroy() {}

};

#endif