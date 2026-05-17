func main(): i32 {
    let x = 10;     // x equals 10
    x = 15;         // x equals 15
    x += 2;         // x equals 17
    let y = x = 10; // x and y equals 10
    return y - x;   // returns 0;
}
