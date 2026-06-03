func main(): u32 {
    return collatz_count(19); // returns 20
}

func collatz_count(n: i32): u32 {
    let count = 0u32;
    for n != 1; count += 1 {
        if n % 2 == 0 {
            n /= 2;
        } else {
            n = n * 3 + 1;
        }
    }
    return count;
}
