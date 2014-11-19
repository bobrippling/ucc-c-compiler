extern int printf(const char *, ...);

#define MALLOC2(x) (void *)malloc2(x)
struct list
{
	int i, init;
	struct list *next;
};

long malloc2(unsigned sz)
{
	static char buf[256];
	static char *pos;
	char *ret;

	if(!pos)
		pos = buf;

	ret = pos;
	pos += sz;
	return (long)ret;
}

list_add(struct list *h, int v)
{
	while(h->init)
		h = h->next;
	h->i = v;
	h->init = 1;
	h->next = MALLOC2(24);
}

list_print(struct list *h)
{
	while(h->init){
		printf("%d\n", h->i);
		h = h->next;
	}
}

main()
{
	struct list *head = MALLOC2(24); //sizeof(struct list)); //sizeof *head);

	printf("head = %p\n", head);

	head->init = 0;

	list_add(head, 2);
	list_add(head, 5);
	list_add(head, 1);

	list_print(head);
}
