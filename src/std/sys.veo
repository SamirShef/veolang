extern "C" {
    pub func putchar(c: i32): i32;
    pub func strlen(s: *u8): usize;
    pub func write(fd: i32, buf: *u8, count: usize): isize;
    pub func exit(code: i32);
    pub func malloc(size: usize): *u8;
    pub func realloc(ptr: *u8, size: usize): *u8;
    pub func free(ptr: *u8);

    pub func __veo_fs_open(path: *u8, mode: *u8): *u8;
    pub func __veo_fs_close(handle: *u8): i32;
    pub func __veo_fs_read(handle: *u8, buf: *u8, size: usize): isize;
    pub func __veo_fs_write(handle: *u8, buf: *u8, size: usize): isize;
    pub func __veo_fs_seek(handle: *u8, offset: isize, whence: i32): isize;
    pub func __veo_fs_tell(handle: *u8): isize;

    pub func __veo_print_u64(fd: i32, val: u64);
    pub func __veo_print_i64(fd: i32, val: i64);
    pub func __veo_print_f64(fd: i32, val: f64);
}
