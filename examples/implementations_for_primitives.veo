func main(): i32 {
    let x = 'f';
    return x.is_digit(16).(i32); // returns 1 (true)
}

impl char {
    pub func compare_to(x: char): i32 {
        let cur_val = *this;
        if cur_val > x {
            return 1;
        } else if cur_val < x {
            return -1;
        }
        return 0;
    }

    pub func to_upper(): char {
        let val = *this;
        if val >= 'a' && val <= 'z' {
            return val + 'A' - 'a';
        }
        return val;
    }

    pub func to_lower(): char {
        let val = *this;
        if val >= 'A' && val <= 'Z' {
            return val - 'A' + 'a';
        }
        return val;
    }

    pub func is_digit(): bool {
        return this.is_digit(10);
    }

    pub func is_digit(base: i32): bool {
        base = base.clamp(2, 36);
        let val = *this;
        if base <= 10 {
            return val >= '0' && val <= ('0'.(i32) + base - 1).(char);
        }
        return val >= '0' && val <= '9'
            || val.to_lower() >= 'a' && val.to_lower() <= ('a'.(i32) + base - 11).(char);
    }
}

impl i32 {
    pub func clamp(min: i32, max: i32): i32 {
        let val = *this;
        if val < min {
            return min;
        } else if val > max {
            return max;
        }
        return val;
    }
}
