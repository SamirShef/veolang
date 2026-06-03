func main(): i32 {
    return fib(5); // returns 5
}

func fib(x: i32): i32 {
    if x == 0 || x == 1 {
        return x;
    }
    return fib(x - 1) + fib(x - 2);
}
