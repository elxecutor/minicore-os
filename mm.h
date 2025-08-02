#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

// Memory constants
#define PAGE_SIZE 4096
#define KERNEL_HEAP_START 0x00200000  // 2MB - start of kernel heap
#define KERNEL_HEAP_SIZE  0x00100000  // 1MB - size of kernel heap
#define KERNEL_HEAP_END   (KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

// Memory allocation flags
#define ALLOC_KERNEL  0x01
#define ALLOC_USER    0x02
#define ALLOC_ZERO    0x04

// Memory block structure for free list allocator
typedef struct mem_block {
    size_t size;
    int is_free;
    struct mem_block* next;
    struct mem_block* prev;
} mem_block_t;

// Memory statistics structure
typedef struct mem_stats {
    size_t total_memory;
    size_t used_memory;
    size_t free_memory;
    size_t num_allocations;
    size_t num_frees;
    size_t largest_free_block;
} mem_stats_t;

// Page directory and table structures for paging simulation
typedef struct page_entry {
    uint32_t present    : 1;   // Page present in memory
    uint32_t writable   : 1;   // Page is writable
    uint32_t user       : 1;   // Page is accessible by user
    uint32_t reserved1  : 2;   // Reserved bits
    uint32_t accessed   : 1;   // Page has been accessed
    uint32_t dirty      : 1;   // Page has been written to
    uint32_t reserved2  : 2;   // Reserved bits
    uint32_t available  : 3;   // Available for OS use
    uint32_t frame      : 20;  // Frame address (shifted right 12 bits)
} __attribute__((packed)) page_entry_t;

typedef struct page_table {
    page_entry_t pages[1024];
} page_table_t;

typedef struct page_directory {
    page_entry_t tables[1024];
} page_directory_t;

// Multiboot memory map structures
typedef struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

// Memory management functions
void mm_init(void* mmap_addr, uint32_t mmap_length);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void* kcalloc(size_t count, size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t new_size);

// Memory statistics and debugging
mem_stats_t mm_get_stats(void);
void mm_print_stats(void);
void mm_print_memory_map(void);
void mm_debug_heap(void);

// Paging functions (simulation)
void paging_init(void);
page_directory_t* paging_create_directory(void);
int paging_map_page(page_directory_t* dir, uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
int paging_unmap_page(page_directory_t* dir, uint32_t virtual_addr);
uint32_t paging_get_physical_addr(page_directory_t* dir, uint32_t virtual_addr);

// Utility functions
void* memset(void* dest, int value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);

// Memory validation
int mm_validate_pointer(void* ptr);
int mm_check_heap_integrity(void);

#endif // MM_H
