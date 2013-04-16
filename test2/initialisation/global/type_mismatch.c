// RUN: %ucc -c %s
// RUN: %ucc -S -o- %s | grep -F '.long 5'

int *p = 5;

f()
{
	static int *p = 5;
}
