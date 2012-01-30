#ifndef STRUCT_H
#define STRUCT_H

struct struc
{
	char *spel; /* NULL if anon */
	decl **members;
};

int struct_size(struc *);

struc *struct_add(struc ***structs, char *spel, decl **members);
struc *struct_find(struc **structs, const char *spel);

#endif
