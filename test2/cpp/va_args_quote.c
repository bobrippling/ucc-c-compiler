// RUN: %ucc -E %s           |   sed           '/^ *$/d' > %t
// RUN: awk '/MARKER/,/END/' % | sed '1,2d; $d; /^ *$/d' | diff -u - %t
#define QUOTE(a, ...) #a #__VA_ARGS__
#define QUOTE0(...) #__VA_ARGS__

//QUOTE()
QUOTE(one)
QUOTE(a, b, c, d)

QUOTE0()
QUOTE0(a, b)

/*
MARKER
"one" ""
"a" "b, c, d"

""
"a, b"
END
*/
