// expected-no-errors
func foo(x: i32) {}
func foo(x: f64) {}

func main() {
    foo(42);    // foo(i32)
    foo(42.0);  // foo(f64)
}
