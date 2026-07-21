func target(x: i32, y: i64) {}
func target(x: i64, y: i32) {}

func main() {
    target(1, 2); // expected-error: call to 'target' is ambiguous
}
