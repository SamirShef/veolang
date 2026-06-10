func main(): i32 {
    let f1 = Fraction.new(2, 7);
    let f2 = Fraction.new(3, 8);
    f1 = f1.div(f2);
    f1 = f1.reduct();
    return 0;
}

trait Add {
    pub func add(other: This): This;
}

trait Sub {
    pub func sub(other: This): This;
}

trait Mul {
    pub func mul(other: This): This;
}

trait Div {
    pub func div(other: This): This;
}

struct Fraction {
    pub num: i32;
    pub den: i32;
}

impl Fraction {
    pub static func new(num: i32, den: i32): This {
        return This { num: num, den: den };
    }

    pub static func new(num: i32): This {
        return This.new(num, 1);
    }

    // does not mutate base object
    pub func reduct(): This {
        let gcd = this.gcd();
        return This { num: this.num / gcd, den: this.den / gcd };
    }

    // the greatest common divisor
    func gcd(): i32 {
        let divisible = this.num;
        let divider = this.den;
        for {
            let rem = divisible % divider;
            if rem == 0 {
                return divider;
            }
            divisible = divider;
            divider = rem;
        }
    }
}

impl Add for Fraction {
    // does not mutate base object
    pub func add(other: This): This {
        let f1 = *this;
        if this.den != other.den {
            let lastDen = this.den;
            f1 = this.mul(Fraction.new(other.den, other.den));
            other = other.mul(Fraction.new(lastDen, lastDen));
        }
        return This { num: f1.num + other.num, den: f1.den };
    }
}

impl Sub for Fraction {
    // does not mutate base object
    pub func sub(other: This): This {
        let f1 = *this;
        if this.den != other.den {
            let lastDen = this.den;
            f1 = this.mul(Fraction.new(other.den, other.den));
            other = other.mul(Fraction.new(lastDen, lastDen));
        }
        return This { num: f1.num - other.num, den: f1.den };
    }
}

impl Mul for Fraction {
    // does not mutate base object
    pub func mul(other: This): This {
        return This { num: this.num * other.num, den: this.den * other.den };
    }
}

impl Div for Fraction {
    // does not mutate base object
    pub func div(other: This): This {
        return This { num: this.num * other.den, den: this.den * other.num };
    }
}
