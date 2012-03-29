#ifndef STRUCT_ENUM_H
#define STRUCT_ENUM_H

void st_en_un_set_spel(char **dest, char *spel, const char *desc);

void st_en_lookup(void **save_to,
		int *incomplete,
		decl *d,
		symtable *stab,
		void *(*lookup)(symtable *, const char *),
		int is_struct);

void st_en_lookup_chk(decl *d, symtable *stab);

#endif
