// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep globl; [ $? -ne 0 ]

static int f();

int f();

extern int f()
{
	return 3;
}
