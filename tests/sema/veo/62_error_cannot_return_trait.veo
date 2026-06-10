trait Foo {}

// expected-error@+1: cannot use trait 'Foo' as a concrete return type
func foo(): Foo {}
