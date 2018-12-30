// RUN: %ucc -c %s

void f()
{
	"hello";
}

int uninit;

long init_5 = 5;

struct
{
	char c, b;
	int i;
	long l;
} a_uninit, a_init = { 1, 2, 3, 4 };
