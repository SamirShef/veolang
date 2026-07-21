struct Player {
    hp: i32;
}

func main() {
    let p: Player;
    // expected-error@+1: type 'Player' has no field named 'mp'
    let mana = p.mp;
}
