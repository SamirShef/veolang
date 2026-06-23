import std;
import std.io;

func main(): i32 {
    let s = std.String.from("Hello world!");
    io.println(s.data());
    let c = s.get(14uz);
    return c.unwrap();
}
