import std.math;
import std.mem;
import std.sys;

/**
 * @brief standard mutable string container (only ASCII characters)
 */
pub struct String {
    data: *u8;
    len: usize;
    cap: usize;
}

impl String {
    /**
     * @brief allocates String from null-terminated string
     * @param alloc: memory allocator
     * @param str: null-terminated string
     * @return String container
     * @note String data contains null-terminator in the end
     */
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

    /**
     * @brief initialize String from already allocated memory
     * @param data: allocated memory (null-terminated string)
     * @param len: string length
     * @param cap: String capacity
     * @return initialized String
     * @note ownership of data transfers to String
     */
    pub static func init(data: *u8, len: usize, cap: usize): String {
        return String { data: data, len: len, cap: cap };
    }

    /**
     * @brief concatenates String
     * @param alloc: memory allocator
     * @param other: String which concatenates to base string
     * @return base String after concatenation
     * @note other string copies to the end of base string
     * @note base string mutates after concatenation
     * @note writes null-terminator to the end of base string
     */
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

    /**
     * @brief concatenates StringView
     * @param alloc: memory allocator
     * @param other: StringView which concatenates to base string
     * @return base String after concatenation
     * @note other string slice copies to the end of base string
     * @note base string mutates after concatenation
     * @note writes null-terminator to the end of base string
     */
    pub func append(alloc: mem.Allocator, other: StringView) {
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

    /**
     * @brief concatenates null-terminated string
     * @param alloc: memory allocator
     * @param other: null-terminated string which concatenates to base string
     * @return base String after concatenation
     * @note other null-terminated string copies to the end of base string
     * @note base string mutates after concatenation
     * @note writes null-terminator to the end of base string
     */
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

    /**
     * @brief concatenates ASCII character
     * @param alloc: memory allocator
     * @param other: ASCII character which concatenates to base string
     * @return base String after concatenation
     * @note other ASCII character copies to the end of base string
     * @note base string mutates after concatenation
     * @note writes null-terminator to the end of base string
     */
    pub func append(alloc: mem.Allocator, other: u8) {
        if this.cap < this.len + 2uz {
            this.cap = math.max(this.cap * 2uz, this.len + 2uz);
            this.data = alloc.realloc(this.data, this.cap);
        }
        *(this.data + this.len) = other;
        this.len += 1;
        *(this.data + this.len) = 0u8;
    }

    /**
     * @brief frees string data
     * @param alloc: memory allocator
     */
    pub func destroy(alloc: mem.Allocator) {
        alloc.destroy(this.data);
        this.data = nil;
        this.len = 0;
        this.cap = 0;
    }

    /**
     * @brief string data
     * @return string data
     */
    pub func data(): *u8 {
        return this.data;
    }

    /**
     * @brief string length
     * @return string length
     */
    pub func len(): usize {
        return this.len;
    }

    /**
     * @brief sets character to index
     * @param i: index in string
     * @param c: character for set
     * @note: panics of index does not in string bounds
     */
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

    /**
     * @brief gets character to index
     * @param i: index
     * @return optional value (some if index in bounds and none otherwise)
     */
    pub func get(i: usize): OptionU8 {
        if i >= this.len {
            return OptionU8.none();
        }
        return OptionU8.some(*(this.data + i));
    }
}

/**
 * @brief standard immutable string slice (only ASCII characters)
 * @param
 * @return
 * @safety
 */
pub struct StringView {
    data: *u8;
    len: usize;
}

impl StringView {
    /**
     * @brief initialize StringView from allocated null-terminated data
     * @param data: string data
     * @return initialized StringView
     */
    pub static func from(data: *u8): StringView {
        return StringView { data: data, len: sys.strlen(data) };
    }

    /**
     * @brief initialize StringView from allocated null-terminated data
     * @param data: string data
     * @param len: length of data
     * @return initialized StringView
     * @note len may by less then real length of data, but data will not be truncated
     */
    pub static func from(data: *u8, len: usize): StringView {
        return StringView { data: data, len: len };
    }

    /**
     * @brief gets string slice data
     * @return string slice data
     * @note length of data my be greater then slice length
     */
    pub func data(): *u8 {
        return this.data;
    }

    /**
     * @brief gets string slice length
     * @return string slice length
     * @note len may by less then real length of data
     */
    pub func len(): usize {
        return this.len;
    }

    pub func compare_to(other: StringView): i32 {
        if this.len > other.len() {
            return 1;
        } else if this.len < other.len() {
            return -1;
        }
        for let i = 0uz, i < this.len(), i += 1 {
            if *(this.data + i) > *(other.data() + i) {
                return 1;
            } else if *(this.data + i) < *(other.data() + i) {
                return -1;
            }
        }
        return 0;
    }
}

/**
 * @brief standard optional value (for u8 type)
 */
pub struct OptionU8 {
    has_val: bool;
    val: u8;
}

impl OptionU8 {
    /**
     * @brief initialize optional with real value
     * @param val: real value
     * @return optional with real value
     */
    pub static func some(val: u8): OptionU8 {
        return OptionU8 { has_val: true, val: val };
    }

    /**
     * @brief initialize optional without value
     * @return optional without value
     */
    pub static func none(): OptionU8 {
        return OptionU8 { has_val: false };
    }

    /**
     * @brief is has value
     * @return true if has value or false otherwise
     */
    pub func has_val(): bool {
        return this.has_val;
    }

    /**
     * @brief gets value in optional
     * @return value in optional
     * @safety panics if optional does not have value
     */
    pub func unwrap(): u8 {
        if !this.has_val {
            panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    /**
     * @brief gets value in optional
     * @param err_val: value which returns if optional does not have value
     * @return value in optional or err_val if haven't
     */
    pub func unwrap_or(err_val: u8): u8 {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

/**
 * @brief standard optional value (for i32 type)
 */
pub struct OptionI32 {
    has_val: bool;
    val: i32;
}

impl OptionI32 {
    /**
     * @brief initialize optional with real value
     * @param val: real value
     * @return optional with real value
     */
    pub static func some(val: i32): OptionI32 {
        return OptionI32 { has_val: true, val: val };
    }

    /**
     * @brief initialize optional without value
     * @return optional without value
     */
    pub static func none(): OptionI32 {
        return OptionI32 { has_val: false };
    }

    /**
     * @brief is has value
     * @return true if has value or false otherwise
     */
    pub func has_val(): bool {
        return this.has_val;
    }

    /**
     * @brief gets value in optional
     * @return value in optional
     * @safety panics if optional does not have value
     */
    pub func unwrap(): i32 {
        if !this.has_val {
            panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    /**
     * @brief gets value in optional
     * @param err_val: value which returns if optional does not have value
     * @return value in optional or err_val if haven't
     */
    pub func unwrap_or(err_val: i32): i32 {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

/**
 * @brief standard optional value (for String type)
 */
pub struct OptionString {
    has_val: bool;
    val: String;
}

impl OptionString {
    /**
     * @brief initialize optional with real value
     * @param val: real value
     * @return optional with real value
     */
    pub static func some(val: String): OptionString {
        return OptionString { has_val: true, val: val };
    }

    /**
     * @brief initialize optional without value
     * @return optional without value
     */
    pub static func none(): OptionString {
        return OptionString { has_val: false };
    }

    /**
     * @brief is has value
     * @return true if has value or false otherwise
     */
    pub func has_val(): bool {
        return this.has_val;
    }

    /**
     * @brief gets value in optional
     * @return value in optional
     * @safety panics if optional does not have value
     */
    pub func unwrap(): String {
        if !this.has_val {
            panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    /**
     * @brief gets value in optional
     * @param err_val: value which returns if optional does not have value
     * @return value in optional or err_val if haven't
     */
    pub func unwrap_or(err_val: String): String {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

/**
 * @brief panic function
 * @param err: null-terminated error message
 * @note error message prints to stderr
 * @note program terminates with exit code 1
 */
pub func panic(err: *u8) {
    sys.write(2, "veo panic: ", 11uz);
    sys.write(2, err, sys.strlen(err));
    sys.write(2, "\naborting execution...\n", 23uz);
    sys.exit(1);
}

/**
 * @brief dynamic array container for managing multiple String instances
 */
pub struct ListString {
    data: *String;
    len: usize;
    cap: usize;
}

impl ListString {
    /**
     * @brief creates empty list
     * @param alloc: memory allocator
     * @return new empty list
     * @note capacity sets to 4
     */
    pub static func new(alloc: mem.Allocator): ListString {
        let cap = 4uz;
        let data = alloc.alloc(cap * @size_of(String)).(*String);
        return ListString {
            data: data,
            len: 0,
            cap: cap
        };
    }

    /**
     * @brief adds new element to the end of list
     * @param alloc: memory allocator
     * @param val: new element
     * @note ownership of val transfers to list
     * @note if capacity less then needs, it increases by 2 times
     */
    pub func add(alloc: mem.Allocator, val: String) {
        if this.cap < this.len + 1uz {
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = alloc.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(String)
                        ).(*String);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    /**
     * @brief gets element by index
     * @param index: index
     * @return optional element value (some if index in bounds and none otherwise)
     */
    pub func get(index: usize): OptionString {
        if index < this.len {
            return OptionString.some(*(this.data + index));
        }
        return OptionString.none();
    }

    /**
     * @brief list data
     * @return list data
     */
    pub func data(): *String {
        return this.data;
    }

    /**
     * @brief list lenght
     * @return list lenght
     */
    pub func len(): usize {
        return this.len;
    }

    /**
     * @brief list capacity
     * @return list capacity
     */
    pub func capacity(): usize {
        return this.cap;
    }
}

/**
 * @brief trait for types that support explicit conversion into a standard String
 */
pub trait ToString {
    /**
     * @brief converts the implementing type instance to a String representation
     * @param alloc: memory allocator for the resulting String
     * @return the allocated String representation of the object
     */
    pub func to_string(alloc: mem.Allocator): String;
}

/**
 * @brief converts a usize integer value into its standard String representation
 * @param alloc: memory allocator for the allocated String buffer
 * @param val: the unsigned integer value to convert
 * @return the allocated String representation of the number
 */
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

/**
 * @brief converts an i32 integer value into its standard String representation
 * @param alloc: memory allocator for the allocated String buffer
 * @param val: the signed integer value to convert
 * @return the allocated String representation of the number
 */
pub func i32_to_string(alloc: mem.Allocator, val: i32): String {
    if val == 0 {
        return String.from(alloc, "0");
    }

    let s: String;
    let is_neg = false;

    if val < 0 {
        s.append(alloc, '-'.(u8));
        is_neg = true;
    }

    for val != 0 {
        let rem = val % 10;
        if rem < 0 {
            rem = -rem;
        }
        s.append(alloc, '0'.(u8) + rem.(u8));
        val /= 10;
    }

    let left = is_neg.(usize);

    let right = s.len() - 1uz;
    for left < right {
        let tmp = s.get(left).unwrap();
        s.set(left, s.get(right).unwrap());
        s.set(right, tmp);

        left += 1uz;
        right -= 1uz;
    }

    return s;
}

/**
 * @brief checks if the character is an ASCII alphabetic letter
 * @param val: character
 * @return true if the character is within 'A'-'Z' or 'a'-'z', false otherwise
 */
pub func is_ascii_letter(val: char): bool {
    return val >= 'A' && val <= 'Z' || val >= 'a' && val <= 'z';
}

/**
 * @brief checks if the character is an ASCII digit
 * @param val: character
 * @return true if the character is within '0'-'9', false otherwise
 */
pub func is_ascii_digit(val: char): bool {
    return val >= '0' && val <= '9';
}

/**
 * @brief checks if the character is an ASCII alphabetic letter or digit
 * @param val: character
 * @return true if the character is within 'A'-'Z' or 'a'-'z' or '0'-'9', false otherwise
 */
pub func is_ascii_letter_or_digit(val: char): bool {
    return is_ascii_letter(val) || is_ascii_digit(val);
}

/**
 * @brief checks if the character is an ASCII white-space
 * @param val: character
 * @return true if the character is within 9-13 codes or ' ', false otherwise
 */
pub func is_ascii_whitespace(val: char): bool {
    return val >= 9.(char) && val <= 13.(char) || val == ' ';
}
