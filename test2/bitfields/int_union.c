// RUN: %ucc -o %t %s
// RUN: %t | %output_check '00000011' '00000111' '7'

typedef union
{
	int val;
	struct
	{
		unsigned a : 1,
						 b : 1,
						 c : 1,
						 d : 1,
						 e : 1,
						 f : 1,
						 g : 1,
						 h : 1;
	};
} Int;

pInt(Int *p)
{
	printf("%d%d%d%d%d%d%d%d\n",
		p->h,
		p->g,
		p->f,
		p->e,
		p->d,
		p->c,
		p->b,
		p->a);
}


int main()
{
	Int i = { 3 };

	pInt(&i);

	i.c = 1;

	pInt(&i);

	printf("%d\n", i.val);
}
