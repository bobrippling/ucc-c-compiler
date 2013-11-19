#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

/* (type *[]) */
type_ref *parse_type(int newdecl);

decl *parse_decl_single(enum decl_mode mode, int newdecl);

/* type ident(, ident, ...) - multiple of the above */
decl **parse_decls_one_type(int newdecl);

/* type ident...; */
int parse_decls_single_type(
		enum decl_multi_mode mode,
		int newdecl,
		symtable *scope,
		decl ***pdecls);

/* multiple of the above */
void parse_decls_multi_type(
		enum decl_multi_mode mode,
		symtable *scope,
		decl ***pnew);

funcargs *parse_func_arglist(symtable *scope);

decl_init *parse_initialisation(void); /* expr or {{...}} */

int parse_at_decl(void);

void parse_add_attr(decl_attr **append);

#endif
