// expected-no-errors
func process(x: i16) {}
func process(x: i32) {}

func main() {
    process(1); // process(i32)
}
