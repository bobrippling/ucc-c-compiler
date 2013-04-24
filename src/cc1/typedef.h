#ifndef TYPEDEF_H
#define TYPEDEF_H

decl *typedef_find_descended_exclude(
		symtable *, const char *spel, int *pdescended, decl *exclude);

decl *typedef_find_descended(
		symtable *, const char *spel, int *pdescended);

int typedef_visible(symtable *, const char *spel);

#endif
