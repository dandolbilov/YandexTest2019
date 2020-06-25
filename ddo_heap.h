///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:    ddo_heap.h
//  Author:  Daniil Dolbilov
//  Created: 2019-04-20
//
//  Heap implementation for Embedded systems.
//    * define HEAP_BLOCK_SIZE and HEAP_TOTAL_SIZE at compilation time
//    * store the heap in static buffer
//    * store the heap descriptor (allocated blocks info) outside the static buffer
//    * can allocate N (1..X) blocks at a time
//    * can truncate N allocated blocks to M (1..N-1) allocated blocks
//    * can free N/M allocated blocks
//    * can calculate current size of block (block sequence)
//    * can calculate current available memory in the heap
//    * RAM size priority > FLASH (Code) size priority > speed priority
//
//  This is my solution for Yandex candidate test (https://yandex.ru/jobs/vacancies/dev/embedded_systems_dev_drone/).
//
//  Heap descriptor size is fixed for [variant 3], it's easy to place heap descriptor inside the destination static buffer too.
//
//  Look at "README" for detailed solution description.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DDO_HEAP_H_
#define _DDO_HEAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// parameters
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HEAP_BLOCK_SIZE
#define HEAP_BLOCK_SIZE  64
#endif

#ifndef HEAP_TOTAL_SIZE
#define HEAP_TOTAL_SIZE  8192
#endif

#define HEAP_BLOCK_COUNT (HEAP_TOTAL_SIZE / HEAP_BLOCK_SIZE)

///////////////////////////////////////////////////////////////////////////////////////////////////
// interface
///////////////////////////////////////////////////////////////////////////////////////////////////

void  heap_init(uint8_t * p_base_addr);
void* heap_alloc(uint32_t n_bytes);
void  heap_free(void * p_block);

void  heap_truncate(void * p_block, uint32_t n_bytes);
uint32_t  heap_block_size(void * p_block);
uint32_t  heap_available_space(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
// implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

#define HEAP_PAGE_BLOCKS 4
#define HEAP_PAGE_COUNT  (HEAP_BLOCK_COUNT / HEAP_PAGE_BLOCKS)

#define BIT_BLOCK_ALLOCATED(iBlock) ((uint8_t)(0x01 << iBlock * 2))
#define BIT_BLOCK_HAS_NEXT(iBlock)  ((uint8_t)(0x02 << iBlock * 2))
#define BIT_BLOCK_ALL_BITS(iBlock)  ((uint8_t)(0x03 << iBlock * 2))

typedef union
{
    struct
    {
        uint8_t block0_allocated :1;  /* bit 0 */
        uint8_t block0_has_next  :1;  /* bit 1 */
        uint8_t block1_allocated :1;  /* bit 2 */
        uint8_t block1_has_next  :1;  /* bit 3 */
        uint8_t block2_allocated :1;  /* bit 4 */
        uint8_t block2_has_next  :1;  /* bit 5 */
        uint8_t block3_allocated :1;  /* bit 6 */
        uint8_t block3_has_next  :1;  /* bit 7 */
    } b;

    uint8_t page_raw;

} Page_t;

///////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint8_t * p_base_addr;
    Page_t    pages[HEAP_PAGE_COUNT];

} Heap_t;

///////////////////////////////////////////////////////////////////////////////////////////////////

void  heap_init_impl(uint8_t * p_base_addr, Heap_t * pHeap);
void* heap_alloc_impl(Heap_t * pHeap, uint32_t n_bytes);
void  heap_free_impl(Heap_t * pHeap, void * p_block);

void  heap_truncate_impl(Heap_t * pHeap, void * p_block, uint32_t n_bytes);
uint32_t  heap_block_size_impl(Heap_t * pHeap, void * p_block);
uint32_t  heap_available_space_impl(Heap_t * pHeap);

///////////////////////////////////////////////////////////////////////////////////////////////////
// helpers
///////////////////////////////////////////////////////////////////////////////////////////////////

void set_allocation_state(Heap_t * pHeap, uint32_t first_block, uint32_t block_count, bool allocate);

uint32_t block_number_by_addr(Heap_t * pHeap, void * p_block);


#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif // _DDO_HEAP_H_
