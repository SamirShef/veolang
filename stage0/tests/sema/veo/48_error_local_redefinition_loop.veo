func main() {
    for let i: i32 = 0, i < 10, i = i + 1 {
        // expected-error@+1: variable 'i' is already defined
        let i: f64 = 4.4;
    }
}
