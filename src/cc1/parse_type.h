#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

#include "../util/compiler.h"

enum decl_mode
{
	DECL_SPEL_NEED    = 1 << 0,
	DECL_CAN_DEFAULT  = 1 << 1,
	DECL_ALLOW_STORE  = 1 << 2
};

enum decl_multi_mode
{
	DECL_MULTI_CAN_DEFAULT        = 1 << 0,
	DECL_MULTI_ACCEPT_FIELD_WIDTH = 1 << 1,
	DECL_MULTI_ACCEPT_FUNC_DECL   = 1 << 2,
	DECL_MULTI_ACCEPT_FUNC_CODE   = 1 << 3 | DECL_MULTI_ACCEPT_FUNC_DECL,
	DECL_MULTI_ALLOW_STORE        = 1 << 4,
	DECL_MULTI_NAMELESS           = 1 << 5,
	DECL_MULTI_ALLOW_ALIGNAS      = 1 << 6,
	DECL_MULTI_IS_OLD_ARGS        = 1 << 7,

	/* another name for field-width, for documentation/checking */
	DECL_MULTI_IS_STRUCT_UN_MEMB  = DECL_MULTI_ACCEPT_FIELD_WIDTH
};


/* (type *[]) */
type *parse_type(int newdecl_ctx, symtable *scope);

/* type *name[]... */
decl *parse_decl(
		enum decl_mode mode, int newdecl_ctx,
		symtable *scope, symtable *add_to_scope);

/* type ident...; */
int parse_decl_group(
		enum decl_multi_mode mode,
		int newdecl_ctx,
		symtable *in_scope,
		symtable *add_to_scope, decl ***pdecls)
	ucc_nonnull((3));

struct funcargs *parse_func_arglist(symtable *);

int parse_at_decl(symtable *scope, int include_attribute);

void parse_add_attr(
		attribute ***append,
		symtable *scope,
		enum attribute_category cat);

type **parse_type_list(symtable *scope);

#endif
