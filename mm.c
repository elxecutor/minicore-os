#include "mm.h"

// Global memory management state
static mem_block_t* heap_head = NULL;
static void* heap_start = NULL;
static size_t heap_size = 0;
static mem_stats_t mem_stats = {0};

// Simulated page directory (for demonstration)
static page_directory_t kernel_page_directory;
static page_table_t kernel_page_tables[256]; // Support for 1GB of virtual memory

// Simple implementations of standard library functions
void* memset(void* dest, int value, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    while (count--) {
        *d++ = (unsigned char)value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// Initialize memory management
void mm_init(void* mmap_addr, uint32_t mmap_length) {
    // Set up heap
    heap_start = (void*)KERNEL_HEAP_START;
    heap_size = KERNEL_HEAP_SIZE;
    
    // Initialize the first block
    heap_head = (mem_block_t*)heap_start;
    heap_head->size = heap_size - sizeof(mem_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
    heap_head->prev = NULL;
    
    // Initialize statistics
    mem_stats.total_memory = heap_size;
    mem_stats.free_memory = heap_head->size;
    mem_stats.used_memory = 0;
    mem_stats.num_allocations = 0;
    mem_stats.num_frees = 0;
    mem_stats.largest_free_block = heap_head->size;
    
    // Initialize paging structures
    paging_init();
    
    // Parse multiboot memory map if provided
    if (mmap_addr && mmap_length > 0) {
        // For now, we'll just acknowledge it exists
        // In a full implementation, we'd parse this to understand available memory
    }
}

// Find a free block that can fit the requested size
static mem_block_t* find_free_block(size_t size) {
    mem_block_t* current = heap_head;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Split a block if it's larger than needed
static void split_block(mem_block_t* block, size_t size) {
    if (block->size > size + sizeof(mem_block_t) + 32) { // Minimum split size
        mem_block_t* new_block = (mem_block_t*)((char*)block + sizeof(mem_block_t) + size);
        new_block->size = block->size - size - sizeof(mem_block_t);
        new_block->is_free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        
        block->next = new_block;
        block->size = size;
    }
}

// Merge adjacent free blocks
static void merge_free_blocks(mem_block_t* block) {
    // Merge with next block
    while (block->next && block->next->is_free) {
        mem_block_t* next = block->next;
        block->size += next->size + sizeof(mem_block_t);
        block->next = next->next;
        if (next->next) {
            next->next->prev = block;
        }
    }
    
    // Merge with previous block
    while (block->prev && block->prev->is_free) {
        mem_block_t* prev = block->prev;
        prev->size += block->size + sizeof(mem_block_t);
        prev->next = block->next;
        if (block->next) {
            block->next->prev = prev;
        }
        block = prev;
    }
}

// Allocate memory
void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    mem_block_t* block = find_free_block(size);
    if (!block) {
        return NULL; // Out of memory
    }
    
    // Split the block if necessary
    split_block(block, size);
    
    // Mark as used
    block->is_free = 0;
    
    // Update statistics
    mem_stats.used_memory += block->size;
    mem_stats.free_memory -= block->size;
    mem_stats.num_allocations++;
    
    // Return pointer to data (after the header)
    return (char*)block + sizeof(mem_block_t);
}

// Allocate aligned memory
void* kmalloc_aligned(size_t size, size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return NULL; // Alignment must be power of 2
    }
    
    // Allocate extra space for alignment
    size_t total_size = size + alignment + sizeof(void*);
    void* raw_ptr = kmalloc(total_size);
    if (!raw_ptr) {
        return NULL;
    }
    
    // Calculate aligned address
    uintptr_t aligned_addr = ((uintptr_t)raw_ptr + sizeof(void*) + alignment - 1) & ~(alignment - 1);
    void* aligned_ptr = (void*)aligned_addr;
    
    // Store original pointer before aligned pointer
    *((void**)aligned_ptr - 1) = raw_ptr;
    
    return aligned_ptr;
}

// Allocate zeroed memory
void* kcalloc(size_t count, size_t size) {
    size_t total_size = count * size;
    if (total_size / count != size) {
        return NULL; // Overflow check
    }
    
    void* ptr = kmalloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

// Free memory
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // Get the block header
    mem_block_t* block = (mem_block_t*)((char*)ptr - sizeof(mem_block_t));
    
    // Validate the block
    if (block->is_free) {
        return; // Double free
    }
    
    // Mark as free
    block->is_free = 1;
    
    // Update statistics
    mem_stats.used_memory -= block->size;
    mem_stats.free_memory += block->size;
    mem_stats.num_frees++;
    
    // Merge with adjacent free blocks
    merge_free_blocks(block);
}

// Reallocate memory
void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) {
        return kmalloc(new_size);
    }
    
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    mem_block_t* block = (mem_block_t*)((char*)ptr - sizeof(mem_block_t));
    
    if (block->size >= new_size) {
        // Current block is large enough
        split_block(block, new_size);
        return ptr;
    }
    
    // Need to allocate new block
    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size < new_size ? block->size : new_size);
        kfree(ptr);
    }
    
    return new_ptr;
}

// Get memory statistics
mem_stats_t mm_get_stats(void) {
    // Update largest free block
    mem_stats.largest_free_block = 0;
    mem_block_t* current = heap_head;
    
    while (current) {
        if (current->is_free && current->size > mem_stats.largest_free_block) {
            mem_stats.largest_free_block = current->size;
        }
        current = current->next;
    }
    
    return mem_stats;
}

// Initialize paging structures
void paging_init(void) {
    // Clear page directory
    memset(&kernel_page_directory, 0, sizeof(page_directory_t));
    
    // Map first 4MB of memory (identity mapping)
    for (int i = 0; i < 1024; i++) {
        uint32_t physical_addr = i * PAGE_SIZE;
        
        // Set up page entry
        kernel_page_tables[0].pages[i].present = 1;
        kernel_page_tables[0].pages[i].writable = 1;
        kernel_page_tables[0].pages[i].user = 0;
        kernel_page_tables[0].pages[i].frame = physical_addr >> 12;
    }
    
    // Install page table in directory
    kernel_page_directory.tables[0].present = 1;
    kernel_page_directory.tables[0].writable = 1;
    kernel_page_directory.tables[0].user = 0;
    kernel_page_directory.tables[0].frame = (uint32_t)&kernel_page_tables[0] >> 12;
}

// Validate pointer
int mm_validate_pointer(void* ptr) {
    if (!ptr) {
        return 0;
    }
    
    // Check if pointer is within heap bounds
    if (ptr < heap_start || ptr >= (char*)heap_start + heap_size) {
        return 0;
    }
    
    return 1;
}

// Check heap integrity
int mm_check_heap_integrity(void) {
    mem_block_t* current = heap_head;
    size_t total_size = 0;
    
    while (current) {
        // Check for corruption
        if ((char*)current < (char*)heap_start || 
            (char*)current >= (char*)heap_start + heap_size) {
            return 0; // Corruption detected
        }
        
        total_size += current->size + sizeof(mem_block_t);
        
        // Check next/prev consistency
        if (current->next && current->next->prev != current) {
            return 0; // Corruption detected
        }
        
        current = current->next;
    }
    
    return total_size <= heap_size;
}

// Print memory statistics
void mm_print_stats(void) {
    mem_stats_t stats = mm_get_stats();
    
    // We'll need to add these print functions to kernel.c
    // For now, these are placeholder implementations
    extern void terminal_writestring(const char* data);
    extern void terminal_write_hex(uint32_t value);
    extern void terminal_write_dec(uint32_t value);
    
    terminal_writestring("=== Memory Statistics ===\n");
    terminal_writestring("Total Memory: ");
    terminal_write_dec(stats.total_memory);
    terminal_writestring(" bytes\n");
    
    terminal_writestring("Used Memory: ");
    terminal_write_dec(stats.used_memory);
    terminal_writestring(" bytes\n");
    
    terminal_writestring("Free Memory: ");
    terminal_write_dec(stats.free_memory);
    terminal_writestring(" bytes\n");
    
    terminal_writestring("Allocations: ");
    terminal_write_dec(stats.num_allocations);
    terminal_writestring("\n");
    
    terminal_writestring("Frees: ");
    terminal_write_dec(stats.num_frees);
    terminal_writestring("\n");
    
    terminal_writestring("Largest Free Block: ");
    terminal_write_dec(stats.largest_free_block);
    terminal_writestring(" bytes\n");
}

// Print memory map
void mm_print_memory_map(void) {
    extern void terminal_writestring(const char* data);
    extern void terminal_write_hex(uint32_t value);
    
    terminal_writestring("=== Memory Map ===\n");
    terminal_writestring("Kernel Heap Start: 0x");
    terminal_write_hex(KERNEL_HEAP_START);
    terminal_writestring("\n");
    
    terminal_writestring("Kernel Heap End: 0x");
    terminal_write_hex(KERNEL_HEAP_END);
    terminal_writestring("\n");
    
    terminal_writestring("Heap Size: ");
    terminal_write_hex(KERNEL_HEAP_SIZE);
    terminal_writestring(" bytes\n");
}

// Debug heap structure
void mm_debug_heap(void) {
    extern void terminal_writestring(const char* data);
    extern void terminal_write_hex(uint32_t value);
    extern void terminal_write_dec(uint32_t value);
    
    terminal_writestring("=== Heap Debug ===\n");
    
    mem_block_t* current = heap_head;
    int block_count = 0;
    
    while (current && block_count < 20) { // Limit output
        terminal_writestring("Block ");
        terminal_write_dec(block_count);
        terminal_writestring(": Addr=0x");
        terminal_write_hex((uint32_t)current);
        terminal_writestring(", Size=");
        terminal_write_dec(current->size);
        terminal_writestring(", ");
        terminal_writestring(current->is_free ? "FREE" : "USED");
        terminal_writestring("\n");
        
        current = current->next;
        block_count++;
    }
    
    if (current) {
        terminal_writestring("... (more blocks)\n");
    }
}
