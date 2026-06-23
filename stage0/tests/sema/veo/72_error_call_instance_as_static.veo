struct Tool { id: i32; }

impl Tool {
    pub func run() {}
}

func main() {
    // expected-error@+1: instance method 'run' cannot be accessed through a type
    Tool.run();
}
