// RUN: %debug_check %s

struct A
{
	unsigned a : 1,
		b : 1,
		c : 1,
		d : 2,
		e : 27;

	signed x : 1;
	int : 0;
	signed y : 5;
	signed z : 27;

} bf = {
	1,
	0,
	1,
	0b10,
	0xff3355,

	-1,

	3,
	7
};

main()
{
}
