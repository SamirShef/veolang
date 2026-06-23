func main() {
    if true {
        let y = true;
        let y = false; // expected-error: variable 'y' is already defined
    }
}
