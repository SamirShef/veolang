import std;
import std.mem;
import std.sys;

pub struct File {
    handle: *u8;
}

impl File {
    pub static func open(path: *u8, mode: *u8): File {
        let h = sys.__veo_fs_open(path, mode);
        return File { handle: h };
    }

    pub func is_open(): bool {
        return this.handle != nil;
    }

    pub func close(): i32 {
        if this.handle == nil {
            return 0;
        }
        let res = sys.__veo_fs_close(this.handle);
        this.handle = nil;
        return res;
    }

    pub func read(buf: *u8, size: usize): isize {
        if this.handle == nil {
            return -1.(isize);
        }
        return sys.__veo_fs_read(this.handle, buf, size);
    }

    pub func write(buf: *u8, size: usize): isize {
        if this.handle == nil {
            return -1iz;
        }
        return sys.__veo_fs_write(this.handle, buf, size);
    }

    pub func size(): isize {
        if this.handle == nil {
            return -1iz;
        }

        let current = sys.__veo_fs_tell(this.handle);
        sys.__veo_fs_seek(this.handle, 0.(isize), 2); // SEEK_END
        let total = sys.__veo_fs_tell(this.handle);
        sys.__veo_fs_seek(this.handle, current, 0);   // SEEK_SET

        return total;
    }

    pub func read_all(alloc: mem.Allocator): std.String {
        if !this.is_open() {
            return std.String.from(alloc, "");
        }

        let file_size = this.size();
        if file_size <= 0iz {
            return std.String.from(alloc, "");
        }

        let size = file_size.(usize);

        let raw_data = alloc.alloc(@size_of(u8) * (size + 1uz));
        let read_bytes = sys.__veo_fs_read(this.handle, raw_data, size);

        if read_bytes < 0iz {
            alloc.destroy(raw_data);
            return std.String.from(alloc, "");
        }

        let actual_len = read_bytes.(usize);
        *(raw_data + actual_len) = 0u8;

        return std.String.init(
            raw_data,
            actual_len,
            size + 1uz
        );
    }
}
