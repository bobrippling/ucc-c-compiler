#ifndef SYM_FOLD_H
#define SYM_FOLD_H

void symtab_fold_sues(symtable *);

void symtab_fold_decls(symtable *);

unsigned symtab_layout_decls(
		symtable *, unsigned current);

#endif
