// expected-no-errors
func print_wide(x: i64) {}

func main() {
    let short_val = 10;
    print_wide(short_val); // implicitly cast i32 -> i64
}
