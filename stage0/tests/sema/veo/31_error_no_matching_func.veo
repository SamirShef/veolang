func test(x: i32) {}

func main() {
    test(true); // expected-error: no matching function for call to 'test'
}
