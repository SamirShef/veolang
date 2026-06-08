// expected-no-errors
struct B { pub c: i32; }
struct A { pub b: B; }

func process(a: A): i32 {
    return a.b.c;
}
