struct A { x: i32; }
struct B { x: i32; }

func main() {
    let a: A;
    // expected-error@+1: cannot cast 'A' to 'B'
    let b: B = a.(B);
}
