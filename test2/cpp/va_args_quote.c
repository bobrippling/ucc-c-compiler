// RUN: %ucc -P -E %s | %output_check -w '"one" ""' '"a" "b, c, d"' '""' '"a, b"'
#define QUOTE(a, ...) #a #__VA_ARGS__
#define QUOTE0(...) #__VA_ARGS__

//QUOTE()
QUOTE(one)
QUOTE(a, b, c, d)

QUOTE0()
QUOTE0(a, b)
