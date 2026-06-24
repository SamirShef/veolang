extern "C" {
    pub func putchar(c: i32): i32;
    pub func strlen(s: *u8): usize;
    pub func write(fd: i32, buf: *u8, count: usize): isize;
    pub func exit(code: i32);
    pub func malloc(size: usize): *u8;
    pub func realloc(ptr: *u8, size: usize): *u8;
    pub func free(ptr: *u8);
}
