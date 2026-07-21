func execute(a: i32, b: f64) {}

func main() {
    // expected-error@+1: no matching function for call to 'execute'
    execute(true, 'z');
}
