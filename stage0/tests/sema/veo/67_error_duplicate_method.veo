struct Dummy {}

impl Dummy {
    func test() {}
    // expected-error@+1: method 'test' is already defined in type 'Dummy'
    func test() {}
}
