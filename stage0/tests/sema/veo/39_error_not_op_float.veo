func main() {
    let invalid_not: bool = !3.14; // expected-error: cannot apply operator '!' with type 'f64'
}
