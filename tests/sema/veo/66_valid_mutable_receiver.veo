// expected-no-errors
struct Wallet {
    pub money: i64;
}

impl Wallet {
    pub func pay(amount: i64) {
        this.money -= amount;
    }
}

func main() {
    let w = Wallet { money: 1000 };
    w.pay(50);
}
