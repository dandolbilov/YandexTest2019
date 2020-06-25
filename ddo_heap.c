///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:    ddo_heap.c
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

#include "ddo_heap.h"
#include <string.h>  // memset

#if 0
#include <stdio.h>
#define LOG_DEBUG printf
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(fmt, ...)
#endif

#ifndef RTOS_CRITICAL_ENTER
#define RTOS_CRITICAL_ENTER
#define RTOS_CRITICAL_EXIT
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// interface
///////////////////////////////////////////////////////////////////////////////////////////////////

static Heap_t g_heap0;

void heap_init(uint8_t * p_base_addr)
{
    heap_init_impl(p_base_addr, &g_heap0);
}

void* heap_alloc(uint32_t n_bytes)
{
    return heap_alloc_impl(&g_heap0, n_bytes);
}

void heap_free(void * p_block)
{
    heap_free_impl(&g_heap0, p_block);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void heap_truncate(void * p_block, uint32_t n_bytes)
{
    heap_truncate_impl(&g_heap0, p_block, n_bytes);
}

uint32_t heap_block_size(void * p_block)
{
    return heap_block_size_impl(&g_heap0, p_block);
}

uint32_t heap_available_space(void)
{
    return heap_available_space_impl(&g_heap0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void heap_init_impl(uint8_t * p_base_addr, Heap_t * pHeap)
{
    pHeap->p_base_addr = p_base_addr;
    memset(&pHeap->pages, 0x00, sizeof(pHeap->pages));
}

void* heap_alloc_impl(Heap_t * pHeap, uint32_t n_bytes)
{
    uint32_t n_blocks = (n_bytes / HEAP_BLOCK_SIZE) + (n_bytes % HEAP_BLOCK_SIZE ? 1 : 0);
    LOG_DEBUG("\nheap_alloc: n_bytes = %u, n_blocks = %u\n", n_bytes, n_blocks);

    uint32_t first_block = 0;
    uint32_t free_blocks = 0;

    RTOS_CRITICAL_ENTER

    for (uint32_t iPage = 0; iPage < HEAP_PAGE_COUNT; ++iPage)
    {
        LOG_DEBUG("heap_alloc: page[%02u] = 0x%02X\n", iPage, pHeap->pages[iPage].page_raw);

        for (uint8_t iBlock = 0; iBlock < HEAP_PAGE_BLOCKS; ++iBlock)
        {
            if (0x00 == (pHeap->pages[iPage].page_raw & BIT_BLOCK_ALLOCATED(iBlock)))  // block is free
            {
                if (!free_blocks) first_block = (iPage * HEAP_PAGE_BLOCKS) + iBlock;
                ++free_blocks;
            }
            else  // block is allocated before
            {
                first_block = 0;
                free_blocks = 0;
            }

            if (free_blocks >= n_blocks)
            {
                LOG_DEBUG("heap_alloc: allocate (first_block = %u, block_count = %u)\n", first_block, n_blocks);
                set_allocation_state(pHeap, first_block, n_blocks, true);

                RTOS_CRITICAL_EXIT
                return pHeap->p_base_addr + first_block * HEAP_BLOCK_SIZE;
            }
        }
    }

    RTOS_CRITICAL_EXIT

    // TODO: allocation failed, maybe add callback here?

    return 0;
}

void heap_free_impl(Heap_t * pHeap, void * p_block)
{
    LOG_DEBUG("\nheap_free: p_block = 0x%04X\n", p_block);

    uint8_t * p_block_addr = (uint8_t*)p_block;
    if (p_block_addr >= pHeap->p_base_addr && p_block_addr <= pHeap->p_base_addr + HEAP_TOTAL_SIZE)
    {
        uint32_t first_block = block_number_by_addr(pHeap, p_block);
        LOG_DEBUG("heap_free: DELETE (first_block = %u)\n", first_block);

        RTOS_CRITICAL_ENTER

        set_allocation_state(pHeap, first_block, 0, false);  // block_count unused, erase until [has_next] = 0b

        RTOS_CRITICAL_EXIT
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void heap_truncate_impl(Heap_t * pHeap, void * p_block, uint32_t n_bytes)
{
    if (heap_block_size_impl(pHeap, p_block) > n_bytes)
    {
        uint32_t first_block = block_number_by_addr(pHeap, p_block);
        uint32_t n_blocks = (n_bytes / HEAP_BLOCK_SIZE) + (n_bytes % HEAP_BLOCK_SIZE ? 1 : 0);

        RTOS_CRITICAL_ENTER

        set_allocation_state(pHeap, first_block, 0, false);        // free old size
        set_allocation_state(pHeap, first_block, n_blocks, true);  // allocate new size

        RTOS_CRITICAL_EXIT
    }
}

uint32_t heap_block_size_impl(Heap_t * pHeap, void * p_block)
{
    uint32_t block_count = 0;

    uint32_t first_block = block_number_by_addr(pHeap, p_block);
    uint32_t first_page = first_block / HEAP_PAGE_BLOCKS;
    uint32_t current_block;

    RTOS_CRITICAL_ENTER

    for (uint32_t iPage = first_page; iPage < HEAP_PAGE_COUNT; ++iPage)
    {
        for (uint8_t iBlock = 0; iBlock < HEAP_PAGE_BLOCKS; ++iBlock)
        {
            current_block = (iPage * HEAP_PAGE_BLOCKS) + iBlock;
            if (current_block >= first_block)
            {
                ++block_count;

                if (0x00 == (pHeap->pages[iPage].page_raw & BIT_BLOCK_HAS_NEXT(iBlock)))  // is last block (chunk)
                {
                    iPage = HEAP_PAGE_COUNT;  // stop external loop
                    break;
                }
            }
        }
    }

    RTOS_CRITICAL_EXIT

    return block_count * HEAP_BLOCK_SIZE;
}

uint32_t heap_available_space_impl(Heap_t * pHeap)
{
    // if it's really useful function => add data member to Heap_t (updated via alloc/free)

    uint32_t free_block_count = 0;

    RTOS_CRITICAL_ENTER

    for (uint32_t iPage = 0; iPage < HEAP_PAGE_COUNT; ++iPage)
    {
        for (uint8_t iBlock = 0; iBlock < HEAP_PAGE_BLOCKS; ++iBlock)
        {
            if (0x00 == (pHeap->pages[iPage].page_raw & BIT_BLOCK_ALLOCATED(iBlock)))  // block is free
            {
                ++free_block_count;
            }
        }
    }

    RTOS_CRITICAL_EXIT

    return free_block_count * HEAP_BLOCK_SIZE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// helpers
///////////////////////////////////////////////////////////////////////////////////////////////////

void set_allocation_state(Heap_t * pHeap, uint32_t first_block, uint32_t block_count, bool allocate)
{
    LOG_DEBUG("set_allocation_state: first = %u, count = %u, state = %u\n", first_block, block_count, allocate);

    uint32_t first_page = first_block / HEAP_PAGE_BLOCKS;
    uint32_t current_block;

    for (uint32_t iPage = first_page; iPage < HEAP_PAGE_COUNT; ++iPage)
    {
        for (uint8_t iBlock = 0; iBlock < HEAP_PAGE_BLOCKS; ++iBlock)
        {
            current_block = (iPage * HEAP_PAGE_BLOCKS) + iBlock;
            if (current_block >= first_block)
            {
                if (allocate && current_block < first_block + block_count)
                {
                    LOG_DEBUG("block[%02u, %02u:%02u] ALLOCATE\n", current_block, iPage, iBlock);
                    pHeap->pages[iPage].page_raw |= BIT_BLOCK_ALL_BITS(iBlock); // [allocated] = 1b, [has_next] = 1b
                    if (current_block == first_block + block_count - 1)  // is last block (chunk)
                    {
                        pHeap->pages[iPage].page_raw &= ~BIT_BLOCK_HAS_NEXT(iBlock); // [has_next] = 0b

                        iPage = HEAP_PAGE_COUNT;  // stop external loop
                        break;
                    }
                }
                else if (!allocate)  // erase
                {
                    LOG_DEBUG("block[%02u, %02u:%02u] DELETE\n", current_block, iPage, iBlock);
                    // detect [has_next] == 0b
                    uint8_t has_next_chunk = pHeap->pages[iPage].page_raw & BIT_BLOCK_HAS_NEXT(iBlock);
                    // free block
                    pHeap->pages[iPage].page_raw &= ~BIT_BLOCK_ALL_BITS(iBlock); // [allocated] = 0b, [has_next] = 0b
                    // go to the next chunk or exit
                    if (!has_next_chunk)
                    {
                        iPage = HEAP_PAGE_COUNT;  // stop external loop
                        break;
                    }
                }
            }
        }
    }
}

uint32_t block_number_by_addr(Heap_t * pHeap, void * p_block)
{
    uint8_t * p_block_addr = (uint8_t*)p_block;
    return (uint32_t)((p_block_addr - pHeap->p_base_addr) / HEAP_BLOCK_SIZE);
}
