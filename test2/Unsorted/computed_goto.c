int main(int argc, char **argv)
{
	void *where[4];
	void *const fin = &&last;
	void *p;
	int i = 0;
	int *next = &&pick;

	where[0] = &&a;
	where[1] = &&b;
	where[2] = &&c;
	where[3] = &&last;

	printf("p = %p (argc=%d), a=%p b=%p c=%p\n", p, argc, &&a, &&b, &&c);

pick:
	p = where[i++];
	goto *p;

a:
	printf("1\n");
	goto *next;
b:
	printf("2\n");
	goto *next;
c:
	printf("3\n");
	goto *next;

last:
	return 0;
}
