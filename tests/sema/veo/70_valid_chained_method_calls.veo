// expected-no-errors
struct Builder {
    id: i32;
}

impl Builder {
    pub func step(): Builder {
        return *this;
    }
}

func main() {
    let b: Builder;
    let final_b = b.step().step();
}
