import std.sys;

pub trait Allocator {
    pub func alloc(size: usize): *u8;
    pub func realloc(ptr: *u8, new_size: usize): *u8;
    pub func destroy(ptr: *u8);
}

pub struct MallocAllocator{}

impl Allocator for MallocAllocator {
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

    pub func destroy(ptr: *u8) {
        sys.free(ptr);
    }
}
