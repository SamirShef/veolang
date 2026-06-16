struct Rect {
    width: i32;
    height: i32;
}

func main(): i32 {
    let rect = Rect.new(2, 5);
    let area = rect.Area();
    return area; // returns 10
}

impl Rect {
    pub static func new(w: i32, h: i32): Rect {
        return Rect { width: w, height: h };
    }

    pub func Area(): i32 {
        return this.width * this.height;
    }
}
