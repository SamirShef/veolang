// expected-no-errors
func main() {
    let p1: *i32 = nil;
    let p2: *u8 = p1.(*u8);
}
