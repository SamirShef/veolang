// expected-no-errors
trait Updatable {
    func update();
}

struct Task {}

impl Updatable for Task {
    func update() {}
}
