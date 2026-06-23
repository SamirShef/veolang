import std.io;

pub struct String {
    data: *u8;
    len: usize;
    cap: usize;
}

impl String {
    pub static func from(str: *u8): This {
        let len = strlen(str);
        let data = malloc(@size_of(u8) * (len + 1uz));
        for let i = 0uz; i < len; i += 1 {
            *(data + i) = *(str + i);
        }
        *(data + len) = 0u8;
        return This { data: data, len: len, cap: len };
    }

    pub func append(other: String) {
        if this.cap < this.len + other.len() {
            this.cap = max(this.cap * 2uz, this.len + other.len());
            this.data = realloc(this.data, this.cap);
        }
        for let i = 0uz; i < other.len(); i += 1 {
            *(this.data + this.len + i) = *(other.data() + i);
        }
        this.len += other.len();
        *(this.data + this.len) = 0u8;
    }

    pub func free() {
        free(this.data);
    }

    pub func data(): *u8 {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func set(i: usize, c: u8) {
        if i >= this.len {
            return;
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

func max(a: usize, b: usize): usize {
    return a >= b ? a : b;
}

extern "C" func strlen(s: *u8): usize;
extern "C" func malloc(size: usize): *u8;
extern "C" func realloc(ptr: *u8, size: usize): *u8;
extern "C" func free(ptr: *u8);

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
    io.print("veo panic: ");
    io.println(err);
    io.println("aborting execution...");
    exit(1);
}

extern "C" func exit(code: i32);
