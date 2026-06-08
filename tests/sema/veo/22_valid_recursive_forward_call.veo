// expected-no-errors
func ping(n: i32) {
    if (n > 0) {
        pong(n - 1);
    }
}

func pong(n: i32) {
    ping(n);
}
