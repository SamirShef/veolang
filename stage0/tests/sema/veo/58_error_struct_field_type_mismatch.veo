struct Color {
    pub r: u8;
    pub g: u8;
    pub b: u8;
}

func main() {
    // expected-error@+1: cannot implicitly cast 'f64' to 'u8'
    let c = Color { r: 255, g: 12.5, b: 0 };
}
