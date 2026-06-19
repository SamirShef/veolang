struct Rect {
    width: i32;
    height: i32;
}

func main(): i32 {
    let rect = Rect.new(2, 5);
<<<<<<< HEAD
    let area = rect.area();
=======
    let area = rect.Area();
>>>>>>> parent of 70c52d1 (Revert "edited examples/implementations.veo")
    return area; // returns 10
}

impl Rect {
<<<<<<< HEAD
    pub static func new(w: i32, h: i32): This {
        return This { width: w, height: h };
=======
    pub static func new(w: i32, h: i32): Rect {
        return Rect { width: w, height: h };
>>>>>>> parent of 70c52d1 (Revert "edited examples/implementations.veo")
    }

    pub func area(): i32 {
        return this.width * this.height;
    }
}
