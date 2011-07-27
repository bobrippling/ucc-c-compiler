#ifndef GEN_ASM_H
#define GEN_ASM_H

void walk_expr(expr *e, symtable *tab);
void walk_tree(tree *);
void walk_fn(function *);

#endif
