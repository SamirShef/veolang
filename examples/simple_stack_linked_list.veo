func main(): i32 {
    let n1 = Node { val: 1, next: nil };
    let n2 = Node { val: 2, next: nil };
    let n3 = Node { val: 3, next: nil };
    let n4 = Node { val: 4, next: nil };
    let n5 = Node { val: 5, next: nil };
    let list = LinkedList.new();
    list.add(&n1);
    list.add(&n2);
    list.add(&n3);
    list.add(&n4);
    list.add(&n5);
    return list.sum(); // returns 15
}

struct Node {
    pub val: i32;
    pub next: *Node;
}

struct LinkedList {
    start: *Node;
    last: *Node;
}

impl LinkedList {
    pub static func new(): This {
        return This {};
    }

    pub func add(n: *Node) {
        if this.start == nil {
            this.start = this.last = n;
        } else {
            this.last.next = n;
            this.last = n;
        }
    }

    pub func sum(): i32 {
        let next = this.start;
        let res = 0;
        for next != nil; {
            res += next.val;
            next = next.next;
        }
        return res;
    }
}

