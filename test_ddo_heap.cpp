///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  File:    test_ddo_heap.c
//  Author:  Daniil Dolbilov
//  Created: 2019-04-20
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "ddo_heap.h"
#include <gtest/gtest.h>


TEST(ddo_heap, alloc_and_free)
{
    static uint8_t heap0_buffer[HEAP_TOTAL_SIZE];

    heap_init(heap0_buffer);

    void* b1 = heap_alloc(HEAP_BLOCK_SIZE * 3);
    void* b2 = heap_alloc(HEAP_BLOCK_SIZE * 3);
    void* b3 = heap_alloc(HEAP_BLOCK_SIZE * 3);

    ASSERT_EQ(heap_available_space(), HEAP_TOTAL_SIZE - HEAP_BLOCK_SIZE * 9);

    heap_free(b2);

    ASSERT_EQ(heap_available_space(), HEAP_TOTAL_SIZE - HEAP_BLOCK_SIZE * 6);

    void* b4 = heap_alloc(HEAP_BLOCK_SIZE * 3);  // in place of b2
    void* b5 = heap_alloc(HEAP_BLOCK_SIZE * 3);

    ASSERT_EQ(b1, heap0_buffer + HEAP_BLOCK_SIZE * 0);
    ASSERT_EQ(b2, heap0_buffer + HEAP_BLOCK_SIZE * 3);
    ASSERT_EQ(b3, heap0_buffer + HEAP_BLOCK_SIZE * 6);

    ASSERT_EQ(b4, heap0_buffer + HEAP_BLOCK_SIZE * 3);  // in place of b2
    ASSERT_EQ(b5, heap0_buffer + HEAP_BLOCK_SIZE * 9);

    ASSERT_EQ(heap_available_space(), HEAP_TOTAL_SIZE - HEAP_BLOCK_SIZE * 12);
}

TEST(ddo_heap, irregular_alloc_size)
{
    static uint8_t heap0_buffer[HEAP_TOTAL_SIZE];

    heap_init(heap0_buffer);

    void* b1 = heap_alloc(HEAP_BLOCK_SIZE * 2);
    void* b2 = heap_alloc(13);  // 13 < 64 (HEAP_BLOCK_SIZE) =>  64 (1 block)
    void* b3 = heap_alloc(85);  // 64 < 85 < 128             => 128 (2 blocks)
    void* b4 = heap_alloc(HEAP_BLOCK_SIZE * 3);

    ASSERT_EQ(b1, heap0_buffer + HEAP_BLOCK_SIZE * 0);
    ASSERT_EQ(b2, heap0_buffer + HEAP_BLOCK_SIZE * 2);  // b2 = 1 block
    ASSERT_EQ(b3, heap0_buffer + HEAP_BLOCK_SIZE * 3);  // b3 = 2 blocks
    ASSERT_EQ(b4, heap0_buffer + HEAP_BLOCK_SIZE * 5);

    ASSERT_EQ(heap_block_size(b1), HEAP_BLOCK_SIZE * 2);
    ASSERT_EQ(heap_block_size(b2), HEAP_BLOCK_SIZE * 1);  // b2 = 1 block
    ASSERT_EQ(heap_block_size(b3), HEAP_BLOCK_SIZE * 2);  // b3 = 2 blocks
    ASSERT_EQ(heap_block_size(b4), HEAP_BLOCK_SIZE * 3);
}

TEST(ddo_heap, big_alloc_fail)
{
    static uint8_t heap0_buffer[HEAP_TOTAL_SIZE];

    heap_init(heap0_buffer);

    void* b1 = heap_alloc(HEAP_BLOCK_SIZE * 2);
    void* b2 = heap_alloc(HEAP_TOTAL_SIZE);  // too big
    void* b3 = heap_alloc(HEAP_BLOCK_SIZE * 3);

    ASSERT_EQ(b1, heap0_buffer + HEAP_BLOCK_SIZE * 0);
    ASSERT_EQ(b2, static_cast<void*>(0));    // alloc failed
    ASSERT_EQ(b3, heap0_buffer + HEAP_BLOCK_SIZE * 2);

    ASSERT_EQ(heap_block_size(b1), HEAP_BLOCK_SIZE * 2);
    ASSERT_EQ(heap_block_size(b2), HEAP_BLOCK_SIZE * 0);  // alloc failed
    ASSERT_EQ(heap_block_size(b3), HEAP_BLOCK_SIZE * 3);
}

TEST(ddo_heap, truncate_block)
{
    static uint8_t heap0_buffer[HEAP_TOTAL_SIZE];

    heap_init(heap0_buffer);

    void* b1 = heap_alloc(HEAP_BLOCK_SIZE * 1);
    void* b2 = heap_alloc(HEAP_BLOCK_SIZE * 5);

    ASSERT_EQ(b1, heap0_buffer + HEAP_BLOCK_SIZE * 0);
    ASSERT_EQ(b2, heap0_buffer + HEAP_BLOCK_SIZE * 1);

    ASSERT_EQ(heap_block_size(b1), HEAP_BLOCK_SIZE * 1);
    ASSERT_EQ(heap_block_size(b2), HEAP_BLOCK_SIZE * 5);

    heap_truncate(b2, HEAP_BLOCK_SIZE * 2);

    ASSERT_EQ(heap_block_size(b2), HEAP_BLOCK_SIZE * 2);

    void* b3 = heap_alloc(HEAP_BLOCK_SIZE * 2);
    void* b4 = heap_alloc(HEAP_BLOCK_SIZE * 1);

    ASSERT_EQ(b3, heap0_buffer + HEAP_BLOCK_SIZE * 3);
    ASSERT_EQ(b4, heap0_buffer + HEAP_BLOCK_SIZE * 5);
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
