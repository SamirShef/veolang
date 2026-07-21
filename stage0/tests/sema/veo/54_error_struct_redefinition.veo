struct Size {
    w: i32;
}

// expected-error@+1: struct 'Size' is already defined
struct Size {
    h: i32;
}
