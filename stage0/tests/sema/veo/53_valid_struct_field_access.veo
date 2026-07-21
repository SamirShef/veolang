// expected-no-errors
struct Node {
    pub id: i64;
    pub active: bool;
}

func check(n: Node): bool {
    return n.active;
}
