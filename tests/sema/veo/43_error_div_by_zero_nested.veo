func main() {
    let complex: i32 = 10 + (20 / (5 - 5)); // expected-error: division by zero
}
