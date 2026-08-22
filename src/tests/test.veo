let a: u8  = 122;
let b: i8  = -124;
let c: u16 = 123;
let d      = 1238i16;
let e: bool = -129 < 0;
let h = -2 + -1;
let i = a;

func main(argc: i32) {
    let a = 10;
    {
        let a = 2;
    }
    return 0;
}

func sum(a: i32, b: i32): i32 {
    return a + b;
}
let sum = sum(2, 3);
