#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/dynarray.h"

#include "sym.h"
#include "cc1.h"

#include "parse_init.h"
#include "parse_expr.h"

#include "tokenise.h"
#include "tokconv.h"

decl_init *parse_init(symtable *scope, int static_ctx)
{
	decl_init *di;

	if(accept(token_open_block)){
		decl_init **exps = NULL;

		di = decl_init_new(decl_init_brace);

#ifdef DEBUG_DECL_INIT
		fprintf(stderr, "new brace %p\n", (void *)di);
#endif

		while(curtok != token_close_block){
			decl_init *sub;
			struct desig *desig = NULL;

			if(curtok == token_open_square || curtok == token_dot){
				/* parse as many as we need */
				struct desig **plast = &desig;

				do{
					struct desig *d = *plast = umalloc(sizeof *d);
					plast = &d->next;

					if(accept(token_dot)){
						d->type = desig_struct;
						d->bits.member = token_current_spel();
						EAT(token_identifier);

					}else if(accept(token_open_square)){
						d->type = desig_ar;
						d->bits.range[0] = parse_expr_exp(scope, static_ctx);

						if(accept(token_elipsis))
							d->bits.range[1] = parse_expr_exp(scope, static_ctx);

						EAT(token_close_square);

					}else{
						ICE("unreachable");
					}
				}while(curtok == token_dot || curtok == token_open_square);

				EAT(token_assign);
			}

			sub = parse_init(scope, static_ctx);
			sub->desig = desig;

			dynarray_add(&exps, sub);

			if(!accept(token_comma))
				break;

			if(curtok == token_close_block && cc1_std < STD_C99)
				warn_at(NULL, "trailing comma in initialiser");
		}

		di->bits.ar.inits = exps;

		EAT(token_close_block);

	}else{
		di = decl_init_new(decl_init_scalar);
		di->bits.expr = PARSE_EXPR_NO_COMMA(scope, static_ctx);
		memcpy_safe(&di->where, &di->bits.expr->where);

#ifdef DEBUG_DECL_INIT
		fprintf(stderr, "new scalar %p\n", (void *)di);
#endif
	}

	return di;
}
