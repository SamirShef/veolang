import std;
import std.sys;

pub func print(str: *u8) {
    sys.write(1, str, sys.strlen(str));
}

pub func print(str: std.String) {
    sys.write(1, str.data(), str.len());
}

pub func print(n: i32) {
    sys.__veo_print_i64(1, n.(i64));
}

pub func println(str: *u8) {
    print(str);
    sys.putchar('\n'.(u8));
}

pub func println(str: std.String) {
    print(str);
    sys.putchar('\n'.(u8));
}

pub func println(n: i32) {
    print(n);
    sys.putchar('\n'.(u8));
}
