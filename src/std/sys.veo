extern "C" {
    /**
     * @brief prints UTF-8 character to stdout
     * @param c: UTF-8 character
     * @return passed character on success or EOF on failure
     */
    pub func putchar(c: i32): i32;

    /**
     * @brief length of null-terminated string
     * @param s: null-terminated string
     * @return length of passed string
     */
    pub func strlen(s: *u8): usize;

    /**
     * @brief writes external buffer of passed size to passed file descriptor
     * @param fd: file descriptor for writing
     * @param buf: buffer which writes to file descriptor
     * @param count: number of bytes to write
     * @return number of bytes written, or -1 if an error occurs.
     */
    pub func write(fd: i32, buf: *u8, count: usize): isize;

    /**
     * @brief terminates program with passed exit code
     * @param code: exit code
     */
    pub func exit(code: i32);

    /**
     * @brief C malloc
     * @param size: size of allocation
     * @return allocated memory or nil on failure
     */
    pub func malloc(size: usize): *u8;

    /**
     * @brief C realloc
     * @param ptr: base pointer for reallocation
     * @param size: new size of base pointer after reallocation
     * @return reallocated base ptr or nil on failure
     */
    pub func realloc(ptr: *u8, size: usize): *u8;

    /**
     * @brief frees allocated memory
     * @param ptr: allocated memory
     */
    pub func free(ptr: *u8);

    /**
     * @brief opens file
     * @param path: path to file (from disk root or current directory)
     * @param mode: mode for opening file (C-standard)
     * @return descriptor of opened file or nil on failure
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_open(path: *u8, mode: *u8): *u8;

    /**
     * @brief closes an open file stream and flushes any unwritten buffered data
     * @param handle: file stream
     * @return 0 on success or EOF on failure
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_close(handle: *u8): i32;

    /**
     * @brief reads opened file content to an external buffer of some size
     * @param handle: file stream
     * @param buf: external buffer for reading
     * @param size: size of content for reading
     * @return number of bytes read on success or -1 if file is closed
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_read(handle: *u8, buf: *u8, size: usize): isize;

    /**
     * @brief writes external buffer of some size into opened file
     * @param handle: file stream
     * @param buf: external buffer for writing
     * @param size: size of content for writing
     * @return number of bytes read on success or -1 if file is closed
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_write(handle: *u8, buf: *u8, size: usize): isize;

    /**
     * @brief moves the file pointer to a specified position within a file
     * @param handle: file stream
     * @return 0 on success or non-zero on failure
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_seek(handle: *u8, offset: isize, whence: i32): isize;

    /**
     * @brief current file pointer position
     * @param handle: file stream
     * @return file pointer position or -1 on failure
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_fs_tell(handle: *u8): isize;

    /**
     * @brief prints u64 number to passed file descriptor
     * @param fd: file descriptor
     * @param val: u64 number for printing
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_print_u64(fd: i32, val: u64);

    /**
     * @brief prints i64 number to passed file descriptor
     * @param fd: file descriptor
     * @param val: i64 number for printing
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_print_i64(fd: i32, val: i64);

    /**
     * @brief prints f64 number to passed file descriptor
     * @param fd: file descriptor
     * @param val: f64 number for printing
     * @note from Veo C-bridge runtime (fs_runtime.c)
     */
    pub func __veo_print_f64(fd: i32, val: f64);
}
