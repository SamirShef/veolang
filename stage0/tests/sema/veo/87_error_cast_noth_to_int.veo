func callback() {}

func main() {
    // expected-error@+1: cannot cast 'noth' to 'i64'
    let addr = callback().(i64);
}
