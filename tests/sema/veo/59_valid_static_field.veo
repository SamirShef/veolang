// expected-no-errors
struct Config {
    pub static version: i32;
}

func get_ver(): i32 {
    return Config.version;
}
