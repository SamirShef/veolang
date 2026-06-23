import std.string;

pub func print(str: *u8) {
    write(1, str, strlen(str));
}

pub func println(str: *u8) {
    let s = string.String.from(str);
    s.set(s.len() - 1uz, 10u8);
    write(1, s.data(), s.len());
}

extern "C" func strlen(s: *u8): usize;
extern "C" func write(fd: i32, buf: *u8, count: usize): isize;
