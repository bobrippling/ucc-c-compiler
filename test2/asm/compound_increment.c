// RUN: %ucc -S -o %t %s
// RUN: [ `grep -c 'call.*f'` -eq 1 ]

int *f();

q()
{
	++*f();
}
