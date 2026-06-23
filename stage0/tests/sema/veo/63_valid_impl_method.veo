// expected-no-errors
struct Rect {
    w: i32;
    h: i32;
}

impl Rect {
    func area(): i32 {
        return this.w * this.h;
    }
}
