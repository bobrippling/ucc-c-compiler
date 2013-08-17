#ifndef SYM_FOLD_H
#define SYM_FOLD_H

void symtab_fold_sues(symtable *);

void symtab_fold_decls(symtable *);

unsigned symtab_layout_decls(
		symtable *, unsigned current);

void symtab_make_syms_and_inits(
		symtable *, stmt **pinit_code);


#endif
