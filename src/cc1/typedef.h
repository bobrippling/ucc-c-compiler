#ifndef TYPEDEF_H
#define TYPEDEF_H

decl *typedef_find_descended_exclude(
		symtable *, const char *spel, int *pdescended, decl *exclude);

decl *scope_find(symtable *, const char *spel);

int typedef_visible(symtable *, const char *spel);

#endif
