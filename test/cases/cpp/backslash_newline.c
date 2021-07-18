// RUN: %ucc -E %s | grep 'hello there'
// RUN: %check %s -E

// spaces follow:
hello \          
there
// CHECK: ^^warning: backslash and newline separated by space
