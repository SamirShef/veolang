func main(): i32 {
    let rect = Rect.new(12, 3);
    let circle = Circle.new(1);
    return calculate_area(rect).(i32); // returns 36
}

trait Area {
    pub func area(): f32;
}

struct Rect {
    w: f32;
    h: f32;
}

struct Circle {
    r: f32;
}

impl Area for Rect {
    pub func area(): f32 {
        return this.w * this.h;
    }
}

impl Rect {
    pub static func new(w: f32, h: f32): Rect {
        return Rect { w: w, h: h };
    }
}

impl Area for Circle {
    pub func area(): f32 {
        return this.r * this.r * 3.1415926535f32;
    }
}

impl Circle {
    pub static func new(r: f32): Circle {
        return Circle { r: r };
    }
}

func calculate_area(a: Area): f32 {
    return a.area();
}
