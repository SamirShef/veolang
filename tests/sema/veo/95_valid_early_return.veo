// expected-no-errors
func check_early(x: i32) {
    if x < 0 {
        return;
    }
    let valid_run = x;
}
