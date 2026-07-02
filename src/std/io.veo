import std;
import std.sys;

/**
 * @brief prints null-terminated string to stdout
 * @param str: null-terminated string
 */
pub func print(str: *u8) {
    sys.write(1, str, sys.strlen(str));
}

/**
 * @brief prints std.String to stdout
 * @param str: std.String string
 */
pub func print(str: std.String) {
    sys.write(1, str.data(), str.len());
}

/**
 * @brief prints std.StringView to stdout
 * @param str: std.StringView string
 */
pub func print(str: std.StringView) {
    sys.write(1, str.data(), str.len());
}

/**
 * @brief prints sign integer to stdout
 * @param n: sign integer number (i32)
 */
pub func print(n: i32) {
    sys.__veo_print_i64(1, n.(i64));
}

/**
 * @brief prints null-terminated string to stdout with new line character ('\n')
 * @param str: null-terminated string
 */
pub func println(str: *u8) {
    print(str);
    sys.putchar('\n'.(u8));
}

/**
 * @brief prints std.String to stdout with new line character ('\n')
 * @param str: std.String string
 */
pub func println(str: std.String) {
    print(str);
    sys.putchar('\n'.(u8));
}

/**
 * @brief prints std.StringView to stdout with new line character ('\n')
 * @param str: std.StringView string
 */
pub func println(str: std.StringView) {
    print(str);
    sys.putchar('\n'.(u8));
}

/**
 * @brief prints sign integer to stdout with new line character ('\n')
 * @param n: sign integer number (i32)
 */
pub func println(n: i32) {
    print(n);
    sys.putchar('\n'.(u8));
}
