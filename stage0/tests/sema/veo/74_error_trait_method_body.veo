trait Printable {
    // expected-error@+1: trait methods cannot have a body
    func print() {}
}
