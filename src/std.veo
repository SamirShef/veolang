import std.math;
import std.sys;

pub struct String {
    data: *u8;
    len: usize;
    cap: usize;
}

impl String {
    pub static func from(str: *u8): This {
        let len = sys.strlen(str);
        let cap = len + 1uz;
        let data = sys.malloc(@size_of(u8) * cap);
        for let i = 0uz; i < len; i += 1 {
            *(data + i) = *(str + i);
        }
        *(data + len) = 0u8;
        return This { data: data, len: len, cap: cap };
    }

    pub func append(other: String) {
        if this.cap < this.len + other.len() {
            this.cap = math.max(this.cap * 2uz, this.len + other.len());
            this.data = sys.realloc(this.data, this.cap + 1uz);
        }
        for let i = 0uz; i < other.len(); i += 1 {
            *(this.data + this.len + i) = *(other.data() + i);
        }
        this.len += other.len();
        *(this.data + this.len) = 0u8;
    }

    pub func append(other: *u8) {
        let other_len = sys.strlen(other);
        if this.cap < this.len + other_len {
            this.cap = math.max(this.cap * 2uz, this.len + other_len);
            this.data = sys.realloc(this.data, this.cap + 1uz);
        }
        for let i = 0uz; i < other_len; i += 1 {
            *(this.data + this.len + i) = *(other + i);
        }
        this.len += other_len;
        *(this.data + this.len) = 0u8;
    }

    pub func append(other: u8) {
        if this.cap < this.len + 1uz {
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(this.data, this.cap + 1uz);
        }
        *(this.data + this.len) = other;
        this.len += 1;
        *(this.data + this.len) = 0u8;
    }

    pub func free() {
        sys.free(this.data);
    }

    pub func data(): *u8 {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func set(i: usize, c: u8) {
        if i >= this.len {
            let msg = String.from("index out of bounds in String.set (index: ");
            msg.append(i.to_string());
            msg.append(", length: ");
            msg.append(this.len.to_string());
            msg.append(")");
            panic(msg.data());
        }
        *(this.data + i) = c;
    }

    pub func get(i: usize): OptionU8 {
        if i >= this.len {
            return OptionU8.none();
        }
        return OptionU8.some(*(this.data + i));
    }
}

pub struct OptionU8 {
    has_val: bool;
    val: u8;
}

impl OptionU8 {
    pub static func some(val: u8): This {
        return This { has_val: true, val: val };
    }

    pub static func none(): This {
        return This { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): u8 {
        if !this.has_val {
            panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: u8): u8 {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub func panic(err: *u8) {
    sys.write(2, "veo panic: ", 11uz);
    sys.write(2, err, sys.strlen(err));
    sys.write(2, "\naborting execution...\n", 23uz);
    sys.exit(1);
}

pub trait ToString {
    pub func to_string(): String;
}

impl ToString for usize {
    pub func to_string(): String {
        let val = *this;
        if val == 0uz {
            return String.from("0");
        }

        let s: String;
        for val != 0uz; {
            s.append('0'.(u8) + (val % 10uz).(u8));
            val /= 10uz;
        }
        for let i = 0uz; i < s.len() / 2uz; i += 1 {
            let i_from_end = s.len() - 1uz - i;
            let tmp = s.get(i).unwrap();
            s.set(i, s.get(i_from_end).unwrap());
            s.set(i_from_end, tmp);
        }
        return s;
    }
}
