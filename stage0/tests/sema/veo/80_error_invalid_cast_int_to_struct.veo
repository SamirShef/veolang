struct Foo { x: i32; }

func main() {
    let num = 100;
    // expected-error@+1: cannot cast 'i32' to 'Foo'
    let f = num.(Foo);
}
