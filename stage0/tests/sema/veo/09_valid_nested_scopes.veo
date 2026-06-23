// expected-no-errors
func main() {
    let outer: i32 = 1;
    if true {
        let inner = outer;
    }
}
