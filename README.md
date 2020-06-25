## Heap implementation for Embedded systems.
* define HEAP_BLOCK_SIZE and HEAP_TOTAL_SIZE at compilation time
* store the heap in static buffer
* store the heap descriptor (allocated blocks info) outside the static buffer
* can allocate N (1..X) blocks at a time
* can truncate N allocated blocks to M (1..N-1) allocated blocks
* can free N/M allocated blocks
* can calculate current size of block (block sequence)
* can calculate current available memory in the heap
* RAM size priority > FLASH (Code) size priority > speed priority

**This is my solution for Yandex candidate test in April 2019 (https://yandex.ru/jobs/vacancies/dev/embedded_systems_dev_drone/).**  
*Guys, you should spend a couple days with algorithms before an interview, don't make my mistake :)  
If you wanna work for a big corporation you need to "eat" some "apples" from the Red-black tree :)  
Even if -Os and LTO is your best friends in the real projects (you have 8 KB of RAM and you must put 100500 features into 64 KB of code memory to create a big value for the company business).*  

**Original requirements:**  
1.1) store the heap in static buffer  
1.2) can allocate 1 block at a time  
1.3) define HEAP_BLOCK_SIZE and HEAP_TOTAL_SIZE at compilation time  
1.4) adapt to n-bit architectures and RTOS  
1.5) add some unit tests  

**My additional requirements:**  
2.1) store the heap descriptor (allocated blocks info) outside the static buffer  
2.2) can allocate N (1..X) blocks at a time  
2.3) can truncate N allocated blocks to M (1..N-1) allocated blocks  
2.4) can free N/M allocated blocks  
2.5) RAM size priority > FLASH (Code) size priority > speed priority  
2.6) can calculate current available memory in the heap  
2.7) use CMake and GTest for build and testing  

**There are 3 basic variants of the heap descriptor:**  
variant 1: linked list of allocated blocks  
variant 2: array of allocated blocks  
variant 3: byte/bit array of all blocks  

According to [2.5] requirement the heap descriptor must be as small as possible.  
The more memory in use, the more important heap descriptor size is (in case if the heap descriptor is located inside the static buffer too).  
Let's estimate the amount of RAM for each heap descriptor variant if memory usage = 80%.  

[HEAP_BLOCK_SIZE = 64, HEAP_TOTAL_SIZE = 8192] => [BLOCK_COUNT = 128]  

**variant 1:**  
has constraint: maximum HEAP_BLOCK_COUNT = 255 (to use uint8_t for block_num)  
linked_list_node = { uint8_t block_num; uint8_t next_block_num; uint8_t chunk_index; }  // 3 bytes for 1 block  
heap descriptor size = (128 * 0.8 * 3) = 309 bytes (variable)  

**variant 2:**  
has constraint: maximum HEAP_BLOCK_COUNT = 255 (to use uint8_t for block_num)  
array_allocated_block = { uint8_t block_num; uint8_t chunk_index; }  // 2 bytes for 1 block  
array_allocated_count = uint8_t  
heap descriptor size = (128 * 0.8 * 2) + 1 = 207 bytes (variable)  

**variant 3:**  
bit_array_block = { uint8_t block0_allocated:1; uint8_t block0_has_next:1; }  // 2 bits for 1 block  
heap descriptor size = (128 * 1.0 * 0.25) = 32 bytes (fixed)  

[BLOCK_COUNT = 65535, uint16_t for block_num]  
variant 1: heap descriptor size = (65535 * 0.8) * (2 + 2 + 1) = 262140 bytes (variable)  
variant 2: heap descriptor size = (65535 * 0.8) * (2 + 1) + 2 = 157286 bytes (variable)  
variant 3: heap descriptor size = (65535 * 1.0) * 0.25 = 16384 bytes (fixed)  

**[variant 3] is selected to implement.**  

Heap descriptor size is fixed for [variant 3], it's easy to place heap descriptor inside the destination static buffer too.  
