#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32

#include <io.h>
#define sys_write _write

#else

#include <unistd.h>
#define sys_write write

#endif

void
__veo_print_char (int fd, int ch) {
    sys_write (fd, (const char *) &ch, sizeof (ch));
}

void
__veo_print_u64 (int fd, uint64_t val) {
    if (val == 0) {
        sys_write (fd, "0", 1);
        return;
    }

    char buf[20];
    int  i = 20;
    while (val > 0) {
        buf[--i] = (char) ('0' + (val % 10));
        val /= 10;
    }
    sys_write (fd, &buf[i], sizeof (buf) - i);
}

void
__veo_print_i64 (int fd, int64_t val) {
    if (val < 0) {
        sys_write (fd, "-", 1);
        __veo_print_u64 (fd, -(uint64_t) val);
    } else {
        __veo_print_u64 (fd, (uint64_t) val);
    }
}

void
__veo_print_f64 (int fd, double val) {
    if (__builtin_isnan (val)) {
        sys_write (fd, "nan", 3);
        return;
    }
    if (__builtin_isinf (val)) {
        if (val < 0) {
            sys_write (fd, "-", 1);
        }
        sys_write (fd, "inf", 3);
        return;
    }

    union {
        double   f;
        uint64_t u;
    } bits = { .f = val };
    if (bits.u >> 63) {
        sys_write (fd, "-", 1);
        val = -val;
    }

    if (val == 0.0) {
        sys_write (fd, "0.000000", 8);
        return;
    }

    int    exp10 = 0;
    double temp  = val;
    if (temp >= 10.0) {
        while (temp >= 10.0) {
            temp /= 10.0;
            ++exp10;
        }
    } else if (temp < 1.0) {
        while (temp < 1.0) {
            temp *= 10.0;
            --exp10;
        }
    }

    int scientific = exp10 < -4 || exp10 >= 6;

    const int precision = 6;

    if (scientific) {
        temp += 0.0000005;
        if (temp >= 10.0) {
            temp /= 10.0;
            ++exp10;
        }

        uint32_t int_part = (uint32_t) temp;
        __veo_print_u64 (fd, int_part);
        sys_write (fd, ".", 1);

        double   frac      = temp - int_part;
        uint64_t frac_part = (uint64_t) (frac * 1000000.0);

        char     frac_buf[6];
        uint64_t t_frac = frac_part;
        for (int i = precision - 1; i >= 0; i--) {
            frac_buf[i] = (char) ('0' + (t_frac % 10));
            t_frac /= 10;
        }
        sys_write (fd, frac_buf, precision);

        sys_write (fd, "e", 1);
        if (exp10 >= 0) {
            sys_write (fd, "+", 1);
        } else {
            sys_write (fd, "-", 1);
            exp10 = -exp10;
        }
        __veo_print_u64 (fd, exp10);

    } else {
        val += 0.0000005;

        uint64_t int_part = (uint64_t) val;
        __veo_print_u64 (fd, int_part);
        sys_write (fd, ".", 1);

        double frac = val - (double) int_part;

        unsigned __int128 frac_fixed = (unsigned __int128) (frac * (double) (1ULL << 60));

        uint64_t frac_part = (uint64_t) ((frac_fixed * 1000000) >> 60);

        char frac_buf[6];
        for (int i = precision - 1; i >= 0; i--) {
            frac_buf[i] = (char) ('0' + (frac_part % 10));
            frac_part /= 10;
        }
        sys_write (fd, frac_buf, precision);
    }
}
