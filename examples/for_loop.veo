func main(): i32 {
    for {                            // aka while true
        break;
    }
    let x = 0;
    for x < 10; {                    // aka while x < 10
        x += 1;
    }
    for let i = 0; i < 10; i += 1 {} // classical for loop
    return 0;
}
