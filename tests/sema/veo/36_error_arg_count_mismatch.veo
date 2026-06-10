func pair(a: i32, b: i32) {}

func main() {
    pair(1); // expected-error: no matching function for call to 'pair'
}
