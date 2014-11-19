#ifndef SYM_FOLD_H
#define SYM_FOLD_H

void symtab_fold_sues(symtable *stab);
void symtab_fold_decls(symtable *tab);

/* struct layout, check for duplicate decls */
void symtab_fold_decls_sues(symtable *);

void symtab_chk_labels(symtable *);

void symtab_check_rw(symtable *);

void symtab_check_static_asserts(symtable *);

void fold_sym_pack_decl(decl *d, unsigned *sz, unsigned *align);

#endif
