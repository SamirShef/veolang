import std;
import std.fs;
import std.mem;
import std.io;
import std.sys;

let alloc: mem.MallocAllocator;

func main(): i32 {
    let main = fs.File.open("src/main.veo", "r");
    if !main.is_open() {
        std.panic("Cannot open file!");
        return 1;
    }

    let content = main.read_all(alloc);

    io.println("File content:");
    io.println(content);

    content.free(alloc);
    main.close();
    return 0;
}
