// RUN: %ucc -S -o %t %s
// RUN: [ `grep -c 'call.*f' < %t` -eq 1 ]

int *f();

q()
{
	++*f();
}
