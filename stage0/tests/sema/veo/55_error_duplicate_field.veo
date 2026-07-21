struct Item {
    weight: i32;
    // expected-error@+1: field 'weight' is already defined
    weight: f64;
}
