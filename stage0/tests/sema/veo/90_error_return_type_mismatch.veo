// expected-error@+1: not all control paths return a value
func get_score(): i32 {
    // expected-error@+1: cannot implicitly cast 'f64' to 'i32'
    return 95.5;
}
