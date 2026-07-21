func main() {
    // expected-error@+1: no matching function for call to 'get_data'
    get_data(42.0);
}

func get_data(x: bool) {}
