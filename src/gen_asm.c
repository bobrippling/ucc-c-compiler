#include "tree.h"
#include "macros.h"


void walk_expr(expr *e)
{
	switch(e->type){
		case expr_identifier:
			do_something_with(e->spel);
			break;

		case expr_val:
			do_something_with(e->val);
			break;

		case expr_op:
			do_something_with(op_to_str(e->op));
			PRINT_IF(e, lhs, printexpr);
			PRINT_IF(e, rhs, printexpr);
			break;

		case expr_str:
			do_something_with(e->spel);
			break;

		case expr_assign:
			do_something_with(e->spel);
			indent++;
			printexpr(e->expr);
			indent--;
			break;

		case expr_funcall:
		{
			expr **iter;

			do_something_with(e->spel);
			indent++;
			if(e->funcargs)
				for(iter = e->funcargs; *iter; iter++){
					indent++;
					printexpr(*iter);
					indent--;
				}
			indent--;
			break;
		}

		default:
			break;
	}
}

void walk_tree(tree *t)
{
	do_something_with(stat_to_str(t->type));

	PRINT_IF(t, lhs,  printtree);
	PRINT_IF(t, rhs,  printtree);
	PRINT_IF(t, expr, printexpr);

	if(t->decls){
		decl **iter;

		for(iter = t->decls; *iter; iter++){
			indent++;
			printdecl(*iter);
			indent--;
		}
	}

	if(t->codes){
		tree **iter;

		for(iter = t->codes; *iter; iter++){
			indent++;
			printtree(*iter);
			indent--;
		}
	}
}

void walk_fn(function *f)
{
	printdecl(f->func_decl);

	PRINT_IF(f, code, printtree)
}
