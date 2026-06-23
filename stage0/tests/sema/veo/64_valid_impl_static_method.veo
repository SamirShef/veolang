// expected-no-errors
struct Factory {}

impl Factory {
    pub static func new(): Factory {
        return Factory {};
    }
}
