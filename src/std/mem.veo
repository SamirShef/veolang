import std.sys;

/**
 * @brief base allocator trait
 */
pub trait Allocator {
    pub func alloc(size: usize): *u8;
    pub func realloc(ptr: *u8, new_size: usize): *u8;
    pub func destroy(ptr: *u8);
}

/**
 * @brief standard allocator based on C allocator
 */
pub struct MallocAllocator{}

impl Allocator for MallocAllocator {
    /**
     * @brief allocates memory
     * @param size: size of memory for allocation
     * @return allocated memory
     * @safety panics if the allocation fails
     */
    pub func alloc(size: usize): *u8 {
        let mem = sys.malloc(size);
        if mem == nil {
            sys.write(2, "veo panic: cannot allocate memory with size ", 44uz);
            sys.__veo_print_u64(2, size.(u64));
            sys.write(2, " bytes\naborting execution...\n", 29uz);
            sys.exit(1);
        }
        return mem;
    }

    /**
     * @brief reallocates memory
     * @param ptr: base pointer for reallocation
     * @param new_size: new size of base pointer after reallocation
     * @return reallocated memory
     * @safety panics if allocation is failure
     */
    pub func realloc(ptr: *u8, new_size: usize): *u8 {
        let mem = sys.realloc(ptr, new_size);
        if mem == nil {
            sys.write(2, "veo panic: cannot allocate memory with size ", 44uz);
            sys.__veo_print_u64(2, new_size.(u64));
            sys.write(2, " bytes\naborting execution...\n", 29uz);
            sys.exit(1);
        }
        return mem;
    }

    /**
     * @brief frees allocated memory
     * @param ptr: allocated memory
     */
    pub func destroy(ptr: *u8) {
        sys.free(ptr);
    }
}
