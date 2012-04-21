#include <stdlib.h>
#include <stdio.h>

typedef struct list { struct list *next; int i; } list;

list *list_new(int i)
{
	list *l = malloc(sizeof *list_new(i));
	l->next = NULL;
	l->i = i;
	return l;
}

list_addto(list *l, int i)
{
	while(l->next)
		l = l->next;

	l->next = list_new(i);
}

list_print(list *l)
{
	if(l){
		printf("%d\n", l->i);
		list_print(l->next);
	}
}

list_sum(list *l)
{
	return l ? l->i + list_sum(l->next) : 0;
}

main()
{
	list sl;
	list *l = list_new(5);

	l->next = &sl;
	sl.next = NULL;
	sl.i = -27;

	list_addto(l, 2);
	list_addto(l, 3);
	list_addto(l, 8);
	list_addto(l, 9);

	list_print(l);

	return list_sum(l);
}
