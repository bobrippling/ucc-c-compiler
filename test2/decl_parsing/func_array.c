// RUN: %ucc %s -o %t
// RUN: %t | %output_check 'p1 1' 'p2 2' 'p3 3' 'p4 4'

(*f[4])(int);

_Static_assert(sizeof(f) == 32, "bad fptr sz");

#define P(n) \
void n(int i){printf("%s %d\n", __func__, i);}

P(p1)
P(p2)
P(p3)
P(p4)

main()
{
	f[0] = p1;
	f[1] = p2;
	f[2] = p3;
	f[3] = p4;

	for(int i = 0; i < 4; i++)
		f[i](i);
}
