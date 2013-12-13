#ifndef SYM_FOLD_H
#define SYM_FOLD_H

void symtab_fold_sues(symtable *stab);
void symtab_fold_decls(symtable *tab);

/* struct layout, check for duplicate decls */
void symtab_fold_decls_sues(symtable *);

void symtab_chk_labels(symtable *);

void symtab_check_rw(symtable *);

unsigned symtab_layout_decls(
		symtable *, unsigned current);

void symtab_check_static_asserts(symtable *);

#endif
