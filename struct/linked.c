struct list
{
	int i, init;
	struct list *next;
};


list_add(struct list *h, int v)
{
	while(h->init)
		h = h->next;
	h->i = v;
	h->init = 1;
	h->next = malloc(16);
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
	struct list *head = malloc(16); //sizeof(struct list)); //sizeof *head);

	printf("head = %p\n", head);

	head->init = 0;

	list_add(head, 2);
	list_add(head, 5);
	list_add(head, 1);

	list_print(head);
}
