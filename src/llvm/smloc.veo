pub struct SMLoc {
    ptr: *u8;
}

/**
 * @brief represents a token or character location within the source buffer
 * @note wraps a raw pointer into the loaded source code for precise diagnostic tracking
 */
impl SMLoc {
    pub static func new(ptr: *u8): SMLoc {
        return SMLoc { ptr: ptr };
    }
}
