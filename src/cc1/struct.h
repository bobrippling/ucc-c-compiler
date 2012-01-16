#ifndef STRUCT_H
#define STRUCT_H

struct struc
{
	char *spel; /* NULL if anon */
	decl **members;
	int size;
};

int struct_member_offset(expr *e);
int struct_size(struc *);

#define STRUCT_SPEL(st) ((st)->spel ? (st)->spel : "<anon>")

#endif
