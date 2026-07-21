struct Calc {}

impl Calc {
    pub func add(x: i32) {}
}

func main() {
    let c: Calc;
    // expected-error@+1: no matching function for call to 'add'
    c.add(true);
}
