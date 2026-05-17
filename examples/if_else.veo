func main(): i32 {
    let x = 10;             // x equals 10
    if x > 10 {
        return 1;
    }
    if (x += 10) == 10 {    // x equals 20
        return 0;
    }
    else {
        return x;           // returns 20
    }
    return 0;
}
