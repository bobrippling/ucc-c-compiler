#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "macros.h"

#include "decl_init.h"

int decl_init_len(decl_init *di)
{
 switch(di->type){
	 case decl_init_scalar:
		 return 1;

	 case decl_init_brace:
		 return dynarray_count((void **)di->bits.inits);
 }
 ICE("decl init bad type");
 return -1;
}

int decl_init_is_const(decl_init *dinit, symtable *stab)
{
	switch(dinit->type){
		case decl_init_scalar:
		{
			expr *e = dinit->bits.expr;
			intval iv;
			enum constyness type;

			FOLD_EXPR(e, stab);
			const_fold(e, &iv, &type);

			dinit->bits.expr = e;

			return type != CONST_NO;
		}

		case decl_init_brace:
		{
			decl_init **i;
			int k = 1;

			for(i = dinit->bits.inits; i && *i; i++){
				int sub_k = decl_init_is_const(*i, stab);

				if(!sub_k)
					k = 0;
			}

			return k;
		}
	}

	ICE("bad decl init");
	return -1;
}

decl_init *decl_init_new(enum decl_init_type t)
{
	decl_init *di = umalloc(sizeof *di);
	where_new(&di->where);
	di->type = t;
	return di;
}

const char *decl_init_to_str(enum decl_init_type t)
{
	switch(t){
		CASE_STR_PREFIX(decl_init, scalar);
		CASE_STR_PREFIX(decl_init, brace);
	}
	return NULL;
}
