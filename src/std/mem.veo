import std.sys;

pub trait Allocator {
    pub func alloc(size: usize): *u8;
    pub func realloc(ptr: *u8, new_size: usize): *u8;
    pub func destroy(ptr: *u8);
}

pub struct MallocAllocator{}

impl Allocator for MallocAllocator {
    pub func alloc(size: usize): *u8 {
        return sys.malloc(size);
    }

    pub func realloc(ptr: *u8, new_size: usize): *u8 {
        return sys.realloc(ptr, new_size);
    }

    pub func destroy(ptr: *u8) {
        sys.free(ptr);
    }
}
