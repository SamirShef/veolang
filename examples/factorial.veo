func main(): i32 {
    return factorial(5); // returns 120
}

func factorial(x: i32): i32 {
    if x <= 1 {
        return 1;
    }
    return x * factorial(x - 1);
}
