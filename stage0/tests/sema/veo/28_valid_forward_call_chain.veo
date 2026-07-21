// expected-no-errors
func step1() {
    step2();
}

func step2() {
    step3();
}

func step3() {}
