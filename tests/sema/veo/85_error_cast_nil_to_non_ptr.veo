func main() {
    // expected-error@+1: cannot assign 'nil' to a non-pointer type 'i64'
    let x: i64 = nil;
}
