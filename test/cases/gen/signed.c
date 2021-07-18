// RUN:   %ucc -S -o- %s | grep seta
// RUN: ! %ucc -S -o- %s | grep setg

typedef unsigned long off_t;

f(off_t a, off_t b)
{
	return a > b; // seta, not setg
}
