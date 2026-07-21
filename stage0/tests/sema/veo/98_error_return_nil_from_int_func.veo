// expected-error@+1: not all control paths return a value
func get_ptr_fallback(): i32 {
    // expected-error@+1: cannot assign 'nil' to a non-pointer type 'i32'
    return nil;
}
