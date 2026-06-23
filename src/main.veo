import std;
import std.io;

func main(): i32 {
    let s = std.String.from("Hello world!");
    io.println(s.data());
    s.set(14uz, 'a'.(u8));
    return 0;
}
