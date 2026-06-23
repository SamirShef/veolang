struct Counter {
    val: i32;
}

impl Counter {
    pub func increment() {
        this.val += 1;
    }
}

func run() {
    const c: Counter;
    // expected-error@+1: method 'increment' cannot be called on an immutable receiver
    c.increment();
}
