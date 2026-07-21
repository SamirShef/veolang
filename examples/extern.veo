// Now supports only C and Veo
extern "C" func puts(str: *u8);

func main(): i32 {
    puts("Hello world!");
    return 0;
}
