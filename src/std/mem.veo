import std.sys;
import std.math;

pub const  B =         1uz;
pub const KB =  B * 1024uz;
pub const MB = KB * 1024uz;
pub const GB = MB * 1024uz;
pub const TB = GB * 1024uz;
pub const PB = TB * 1024uz;

/**
 * @brief base allocator trait
 */
pub trait Allocator {
    pub func alloc(size: usize): *u8;
    pub func realloc(ptr: *u8, old_size: usize, new_size: usize): *u8;
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
     * @param old_size: old size of base pointer after reallocation
     * @param new_size: new size of base pointer after reallocation
     * @return reallocated memory
     * @safety panics if allocation is failure
     */
    pub func realloc(ptr: *u8, old_size: usize, new_size: usize): *u8 {
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

struct Chunk {
    pub next: *Chunk;
    pub cap: usize;
    pub offset: usize;
}

pub struct ArenaAllocator {
    backing: MallocAllocator;
    cur: *Chunk;
    default_chunk_size: usize;
}

impl ArenaAllocator {
    pub static func init(alloc: MallocAllocator, default_chunk_size: usize): ArenaAllocator {
        return ArenaAllocator {
            backing: alloc,
            cur: nil,
            default_chunk_size: default_chunk_size
        };
    }

    func align_up(n: usize): usize {
        let align = 8uz;
        return (n + align - 1uz) / align * align;
    }

    func alloc_chunk(size: usize): *Chunk {
        let header_size    = @size_of(Chunk);
        let required_space = size + header_size;
        let size_to_alloc  = math.max(required_space, this.default_chunk_size);
        let mem            = this.backing.alloc(size_to_alloc);
        let chunk          = mem.(*Chunk);
        chunk.cap          = size_to_alloc;
        chunk.offset       = header_size;
        chunk.next         = this.cur;
        this.cur           = chunk;
        return chunk;
    }

    pub func reset() {
        let cur = this.cur;
        for cur != nil {
            let next = cur.next;
            this.backing.destroy(cur.(*u8));
            cur = next;
        }
        this.cur = nil;
    }
}

impl Allocator for ArenaAllocator {
    pub func alloc(size: usize): *u8 {
        if size == 0uz {
            return nil;
        }

        let aligned = this.align_up(size);
        if this.cur == nil {
            this.alloc_chunk(aligned);
        }

        let chunk = this.cur;
        if chunk.offset + aligned > chunk.cap {
            chunk = this.alloc_chunk(aligned);
        }

        let raw       = chunk.(*u8) + chunk.offset;
        chunk.offset += aligned;
        return raw;
    }

    pub func realloc(ptr: *u8, old_size: usize, new_size: usize): *u8 {
        if ptr == nil {
            return this.alloc(new_size);
        }
        if new_size == 0uz {
            return nil;
        }

        let new = this.alloc(new_size);
        sys.memcpy(new, ptr, old_size);
        return new;
    }

    pub func destroy(ptr: *u8) {}
}
