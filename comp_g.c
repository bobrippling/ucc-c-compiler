int main(int argc, char **argv)
{
	void *where[] = {
		&&a, &&b, &&c
	};
	void *fin = &&last;
	void *p;

	p = where[argc - 1];
	printf("p = %p (argc=%d), a=%p b=%p c=%p\n", p, argc, &&a, &&b, &&c);

	goto *p;

a:
	printf("1 arg\n");
	goto *fin;
b:
	printf("2 args\n");
	goto *fin;
c:
	printf("3 args\n");
	goto *fin;

last:
	return 0;
}
