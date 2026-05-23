struct Person {
    pub        age: i32;            // public field
               height: f32;         // private field
               weight: f32;         // private field
    pub static anyStaticField: i32; // public static field
}

struct AnyStruct {
    pub foo: i32;
    pub public: f32;
}

func main(): i32 {
    let p1: Person;         // all fields will be initialized by zero
    let any = AnyStruct {   // you can use this way only
        foo: 10,            // if all fields are public.
        public: 3.141592f32 // you cannot initialize static or
    };                      // constant fields in this way

    let foo = any.foo;                       // accessing instance field
    let staticField = Person.anyStaticField; // accessing static field
    return 0;
}
