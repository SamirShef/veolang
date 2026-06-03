func main(): i32 {
    let flag = false;
    let res = flag && (infinite_loop() == 0); // infinite_loop() will not be called
    return 0;
}

func infinite_loop(): i32 {
    for {}
    return 0;
}
