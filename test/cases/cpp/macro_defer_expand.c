// RUN: %ucc -E %s -P -o %t
// RUN: %stdoutcheck %s < %t

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(id) id DEFER(EMPTY)()
#define EXPAND(...) __VA_ARGS__

#define A() 123
A() // Expands to 123
DEFER(A)() // Expands to A () because it requires one more scan to fully expand
EXPAND(DEFER(A)()) // Expands to 123, because the EXPAND macro forces another scan

// STDOUT: 123
// STDOUT: A ()
// STDOUT: 123
