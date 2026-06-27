import std.math;
import std.mem;
import std.sys;
import std;

pub struct SourceMgr {
    buffers: std.ListString;
}

impl SourceMgr {
    pub static func new(alloc: mem.Allocator): SourceMgr {
        return SourceMgr {
            buffers: std.ListString.new(alloc)
        };
    }

    pub func add_buffer(alloc: mem.Allocator, buffer: std.String): usize {
        let id = this.buffers.count();
        this.buffers.add(alloc, buffer);
        return id;
    }

    pub func get_buffer(id: usize): std.OptionString {
        return this.buffers.get(id);
    }
}
