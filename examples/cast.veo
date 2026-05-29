func main(): i32 {
    let x = 10.0;   // f64
    let y = x.(u8); // f64 -> u8 (explicit cast)
    return y;       // u8 -> i32 (implicit cast)
}
