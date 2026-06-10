struct Secret {
    data: i32;
}

func leak(s: Secret) {
    // expected-error@+1: field 'data' of struct 'Secret' is private
    let x = s.data;
}
