#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

/* (type *[]) */
type_ref *parse_type(void);

/* (type) (ident) - type with spel + store */
decl  *parse_decl_single(enum decl_mode mode);

/* type ident(, ident, ...) - multiple of the above */
decl **parse_decls_one_type(void);

/* type ident...; type ident...; - multiple of the above */
void parse_decls_multi_type(enum decl_multi_mode mode, decl ***);

funcargs *parse_func_arglist(void);

decl_init *parse_initialisation(void); /* expr or {{...}} */

#endif
