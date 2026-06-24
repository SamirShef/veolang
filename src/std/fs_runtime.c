#include <stdint.h>
#include <stdio.h>

void *
veo_fs_open (const char *path, const char *mode) {
    return (void *) fopen (path, mode);
}

int32_t
veo_fs_close (void *handle) {
    if (!handle) {
        return -1;
    }
    return fclose ((FILE *) handle);
}

int64_t
veo_fs_read (void *handle, void *buf, uint64_t size) {
    if (!handle) {
        return -1;
    }
    return (int64_t) fread (buf, 1, size, (FILE *) handle);
}

int64_t
veo_fs_write (void *handle, const void *buf, uint64_t size) {
    if (!handle) {
        return -1;
    }
    return (int64_t) fwrite (buf, 1, size, (FILE *) handle);
}

int64_t
veo_fs_seek (void *handle, int64_t offset, int32_t whence) {
    if (!handle) {
        return -1;
    }
    return fseek ((FILE *) handle, offset, whence);
}

int64_t
veo_fs_tell (void *handle) {
    if (!handle) {
        return -1;
    }
    return ftell ((FILE *) handle);
}
