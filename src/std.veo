import std.math;
import std.mem;
import std.sys;

pub struct String {
    data: *u8;
    len: usize;
    cap: usize;
}

impl String {
    pub static func from(alloc: mem.Allocator, str: *u8): String {
        let len = sys.strlen(str);
        let cap = len + 1uz;
        let data = alloc.alloc(@size_of(u8) * cap);
        for let i = 0uz, i < len, i += 1 {
            *(data + i) = *(str + i);
        }
        *(data + len) = 0u8;
        return String { data: data, len: len, cap: cap };
    }

    pub static func init(data: *u8, len: usize, cap: usize): String {
        return String { data: data, len: len, cap: cap };
    }

    pub func append(alloc: mem.Allocator, other: String) {
        if this.cap < this.len + other.len() + 1uz {
            this.cap = math.max(this.cap * 2uz, this.len + other.len() + 1uz);
            this.data = alloc.realloc(this.data, this.cap);
        }
        for let i = 0uz, i < other.len(), i += 1 {
            *(this.data + this.len + i) = *(other.data() + i);
        }
        this.len += other.len();
        *(this.data + this.len) = 0u8;
    }

    pub func append(alloc: mem.Allocator, other: *u8) {
        let other_len = sys.strlen(other);
        if this.cap < this.len + other_len + 1uz {
            this.cap = math.max(this.cap * 2uz, this.len + other_len + 1uz);
            this.data = alloc.realloc(this.data, this.cap);
        }
        for let i = 0uz, i < other_len, i += 1 {
            *(this.data + this.len + i) = *(other + i);
        }
        this.len += other_len;
        *(this.data + this.len) = 0u8;
    }

    pub func append(alloc: mem.Allocator, other: u8) {
        if this.cap < this.len + 2uz {
            this.cap = math.max(this.cap * 2uz, this.len + 2uz);
            this.data = alloc.realloc(this.data, this.cap);
        }
        *(this.data + this.len) = other;
        this.len += 1;
        *(this.data + this.len) = 0u8;
    }

    pub func free(alloc: mem.Allocator) {
        alloc.destroy(this.data);
    }

    pub func data(): *u8 {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func set(i: usize, c: u8) {
        if i >= this.len {
            sys.write(2, "veo panic: ", 11uz);
            sys.write(2, "index out of bounds in String.set (index: ", 42uz);
            sys.__veo_print_u64(2, i.(u64));
            sys.write(2, ", length: ", 10uz);
            sys.__veo_print_u64(2, this.len.(u64));
            sys.write(2, ")", 1uz);
            sys.write(2, "\naborting execution...\n", 23uz);
            sys.exit(1);
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
    pub static func some(val: u8): OptionU8 {
        return OptionU8 { has_val: true, val: val };
    }

    pub static func none(): OptionU8 {
        return OptionU8 { has_val: false };
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
    pub func to_string(alloc: mem.Allocator): String;
}

pub func usize_to_string(alloc: mem.Allocator, val: usize): String {
    if val == 0uz {
        return String.from(alloc, "0");
    }

    let s: String;
    for val != 0uz {
        s.append(alloc, '0'.(u8) + (val % 10uz).(u8));
        val /= 10uz;
    }
    for let i = 0uz, i < s.len() / 2uz, i += 1 {
        let i_from_end = s.len() - 1uz - i;
        let tmp = s.get(i).unwrap();
        s.set(i, s.get(i_from_end).unwrap());
        s.set(i_from_end, tmp);
    }
    return s;
}

pub func i32_to_string(alloc: mem.Allocator, val: i32): String {
    if val == 0 {
        return String.from(alloc, "0");
    }

    let s: String;
    if val < 0 {
        s.append(alloc, '-'.(u8));
        val = -val;
    }
    for val != 0 {
        s.append(alloc, '0'.(u8) + (val % 10).(u8));
        val /= 10;
    }
    for let i = 0uz, i < s.len() / 2uz, i += 1 {
        let i_from_end = s.len() - 1uz - i;
        let tmp = s.get(i).unwrap();
        s.set(i, s.get(i_from_end).unwrap());
        s.set(i_from_end, tmp);
    }
    return s;
}
