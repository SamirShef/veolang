struct Math {}

impl Math {
    pub static func abs(x: i32): i32 { return x; }
}

func main(m: Math) {
    // expected-error@+1: static method 'abs' cannot be accessed through an instance
    m.abs(5);
}
