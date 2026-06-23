// expected-no-errors
struct Logger {}

impl Logger {
    pub func log(msg: i32) {}
    pub func log(msg: f64) {}
}

func run(l: Logger) {
    l.log(10);
    l.log(5.5);
}
