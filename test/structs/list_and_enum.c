#include <stdlib.h>
#include <stdio.h>

typedef struct list list;

struct list
{
	enum list_mode
	{
		HEX, OCT, DEC
	} mode;
	int num;
	char *fmt;
	list *next;
};

list *list_new(enum list_mode m, int n)
{
	list *l = malloc(sizeof *l);
	l->mode = m;
	l->num = n;
	l->fmt = NULL;
	l->next = NULL;
	return l;
}

list_append(list *h, list *l)
{
	if(h->next)
		list_append(h->next, l);
	else
		h->next = l;
}

list_fmt(list *l)
{
	switch(l->mode){
#define MAP(t, s)    \
		case t:          \
			l->fmt = s;    \
			break

		MAP(HEX, "0x%x");
		MAP(OCT, "0%o");
		MAP(DEC, "%d");
	}

	if(l->next)
		list_fmt(l->next);
}

list_print(list *l)
{
	for(; l; l = l->next){
		printf(l->fmt, l->num);
		putchar('\n');
	}
}

int main()
{
	list *head;

	head = list_new(HEX, 55);

	list_append(head, list_new(DEC, 5));
	list_append(head, list_new(OCT, 9));
	list_append(head, list_new(HEX, 19));

	list_fmt(head);

	list_print(head);

	return 0;
}
