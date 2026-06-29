import std.math;
import std.mem;
import std.sys;
import std;

pub struct SourceMgr {
    buffers: std.ListString;
}

/**
 * @brief manages source code buffers for compilation units and diagnostics
 */
impl SourceMgr {
    /**
     * @brief constructs a new SourceMgr instance
     * @param alloc: memory allocator for initializing the internal buffer storage
     * @return an initialized SourceMgr container
     */
    pub static func new(alloc: mem.Allocator): SourceMgr {
        return SourceMgr {
            buffers: std.ListString.new(alloc)
        };
    }

    /**
     * @brief appends a new source buffer to the manager
     * @param alloc: memory allocator for growing the internal buffer storage
     * @param buffer: the source string content to be managed
     * @return the unique ID assigned to the newly added buffer
     */
    pub func add_buffer(alloc: mem.Allocator, buffer: std.String): usize {
        let id = this.buffers.len();
        this.buffers.add(alloc, buffer);
        return id;
    }

    /**
     * @brief retrieves a managed source buffer by its unique identifier
     * @param id: the unique ID of the target buffer
     * @return std.OptionString containing the buffer content if found, or none
     */
    pub func get_buffer(id: usize): std.OptionString {
        return this.buffers.get(id);
    }
}
