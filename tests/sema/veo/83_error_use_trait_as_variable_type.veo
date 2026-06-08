trait Renderable {}
struct Mesh {}
impl Renderable for Mesh {}

func draw(m: Mesh) {
    // expected-error@+1: cannot use trait 'Renderable' as a concrete type for variable
    let obj: Renderable;
}
