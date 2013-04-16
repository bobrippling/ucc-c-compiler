#ifndef TYPEDEF_H
#define TYPEDEF_H

decl *typedef_find(symtable *, const char *spel);
decl *typedef_find4(symtable *stab, const char *spel, decl *exclude, int descend);

#endif
