// RUN: %ucc -E %s | grep -F '(1, 2)'
#define X(a) a

X((1, 2))
