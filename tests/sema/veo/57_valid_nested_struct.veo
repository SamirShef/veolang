// expected-no-errors
struct Cpu {
    pub cores: u32;
}

struct Computer {
    pub processor: Cpu;
    ram_gb: u32;
}

func main(): u32 {
    let c: Computer;
    return c.processor.cores;
}
