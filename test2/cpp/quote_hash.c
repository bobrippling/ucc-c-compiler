// RUN: %ucc -E %s | grep '"#" *"abc"' > /dev/null
#define A "abc"
#define Q(x) #x
func(Q(#) A);
