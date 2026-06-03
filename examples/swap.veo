func main(): i32 {
    let a = 10;
    let b = 5;
    swap(&a, &b);
    return b - a; // returns 10 - 5 (=5)
}

func swap(a: *i32, b: *i32) {
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}
