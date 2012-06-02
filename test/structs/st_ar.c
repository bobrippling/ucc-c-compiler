struct Tim
{
	int a;
	int ar[3];
};

p_tim(struct Tim *t)
{
	for(int i = 0; i < 3; i++)
		printf("t->ar[%d] = %d\n", i, t->ar[i]);
}

p_ar(int *b)
{
	for(int i = 0; i < 3; i++)
		printf("b[%d] = %d\n", i, b[i]);
}

main()
{
	struct Tim t;
	int b[3];

	//t.a = 3;
	for(int i = 0; i < 3; i++){
		t.ar[i] = 3 - i;
		b[i] = 5 - i;
	}

	p_ar(b);
	p_tim(&t);
}
