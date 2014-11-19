#include <stdlib.h>

#include "../../util/dynvec.h"

#include "ops.h"
#include "stmt_asm.h"
#include "../__asm.h"
#include "../out/__asm.h"

const char *str_stmt_asm()
{
	return "asm";
}

static void check_constraint(asm_param *param, symtable *stab)
{
	const char *desc = "__asm__() output";

	if(param->is_output){
		fold_inc_writes_if_sym(param->exp, stab);
		fold_expr_nodecay(param->exp, stab);

	}else{
		FOLD_EXPR(param->exp, stab);
		desc = "__asm__() input";
	}

	fold_check_expr(param->exp, FOLD_CHK_NO_ST_UN, desc);
}

void fold_stmt_asm(stmt *s)
{
	asm_param **it;
	int n_inouts = 0;

	for(it = s->bits.asm_args->params; it && *it; it++, n_inouts++){
		asm_param *param = *it;

		check_constraint(param, s->symtab);

		if(param->is_output){
			if(!expr_is_lval(param->exp))
				die_at(&param->exp->where, "asm output not an lvalue");

			fold_inc_writes_if_sym(param->exp, s->symtab);
		}
	}

	/* validate asm string - s->bits.asm_args->cmd */
	if(s->bits.asm_args->extended){
		char *str;

		for(str = s->bits.asm_args->cmd; *str; str++)
			if(*str == '%'){
				if(str[1] == '%'){
					str++;

				}else if(str[1] == '['){
					ICE("TODO: named constraint");

				}else{
					char *ep;
					long pos = strtol(str + 1, &ep, 0);

					if(ep == str + 1)
						die_at(&s->where, "invalid register character '%c', number expected", *ep);

					if(pos >= n_inouts)
						die_at(&s->where, "invalid register index %ld / %d", pos, n_inouts);

					str = ep - 1;
				}
			}
	}
}

static expr *err_operand_to_expr(
		asm_param **params, struct constrained_val *cval,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs)
{
	size_t o, i;

	for(o = 0; o < outputs->n; o++)
		if(&outputs->arr[o] == cval)
			return params[o]->exp;

	for(i = 0; i < inputs->n; i++)
		if(&inputs->arr[i] == cval)
			return params[o + i]->exp;

	return NULL;
}

static int show_asm_error(
		const stmt *s, struct out_asm_error *error,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs)
{
	const where *loc = &s->where;

	if(!error->str)
		return 0;

	if(error->operand){
		expr *err_expr = err_operand_to_expr(
				s->bits.asm_args->params, error->operand,
				outputs, inputs);

		if(err_expr)
			loc = &err_expr->where;
	}

	warn_at_print_error(loc, "%s", error->str);
	gen_had_error = 1;

	free(error->str), error->str = NULL;

	return 1;
}

static void free_release_valarray(
		out_ctx *octx, struct constrained_val_array *container)
{
	out_asm_release_valarray(octx, container);
	free(container->arr);
}

void gen_stmt_asm(const stmt *s, out_ctx *octx)
{
	asm_param **params;
	struct out_asm_error error = { 0 };
	struct constrained_val_array outputs = { 0 }, inputs = { 0 };

	out_comment(octx, "### begin asm(%s) from %s",
			s->bits.asm_args->extended ? ":::" : "",
			where_str(&s->where));

	for(params = s->bits.asm_args->params; params && *params; params++){
		asm_param *param = *params;
		struct constrained_val *new;
		const out_val *generated;

		if(param->is_output){
			generated = NULL;
			new = dynvec_add(&outputs.arr, &outputs.n);
		}else{
			generated = gen_expr(param->exp, octx);
			new = dynvec_add(&inputs.arr, &inputs.n);
		}

		new->ty = param->exp->tree_type;
		new->val = generated;
		error.operand = new;
		new->calculated_constraint = out_asm_calculate_constraint(
				param->constraints, param->is_output, &error);

		if(show_asm_error(s, &error, &outputs, &inputs)){
			/* error - out_inline_asm...() hasn't released.
			 * we need to release and free */
			goto err;
		}
	}

	if(s->bits.asm_args->extended){
		size_t i;
		struct inline_asm_state state;
		decl *fndecl = symtab_func(s->symtab);

		out_inline_asm_ext_begin(octx,
				s->bits.asm_args->cmd,
				&outputs, &inputs,
				s->bits.asm_args->clobbers,
				&s->where,
				fndecl->ref,
				&error,
				&state);

		out_comment(octx, "### assignments to outputs");

		for(params = s->bits.asm_args->params, i = 0;
				params && *params;
				params++, i++)
		{
			asm_param *param = *params;
			if(!param->is_output)
				continue;
			outputs.arr[i].val = gen_expr(param->exp, octx);

			out_inline_asm_ext_output(octx, i, &outputs.arr[i], &state);
		}

		out_inline_asm_ext_end(&state);

		if(show_asm_error(s, &error, &outputs, &inputs)){
			/* as above */
			goto err;
		}
	}else{
		out_inline_asm(octx, s->bits.asm_args->cmd);
	}

	/* success - out_inline_asm...() has released. we just free */
	free(outputs.arr);
	free(inputs.arr);

	out_comment(octx, "### end asm()");
	return;
err:
	free_release_valarray(octx, &outputs);
	free_release_valarray(octx, &inputs);
}

void init_stmt_asm(stmt *s)
{
	s->f_passable = fold_passable_yes;
}

static void style_asm_bits(asm_param *param)
{
	stylef("\"%s\" (", param->constraints);
	IGNORE_PRINTGEN(gen_expr(param->exp, NULL));
	stylef(")");
}

static void style_asm_params(asm_param **i, int outputs)
{
	int comma = 0;

	stylef(" : ");
	for(; i && *i; i++){
		asm_param *param = *i;

		if(param->is_output == outputs){
			if(comma){
				stylef(", ");
				comma = 0;
			}

			style_asm_bits(*i);
			comma = 1;
		}
	}
}

void style_stmt_asm(const stmt *s, out_ctx *octx)
{
	(void)octx;

	stylef(
			"asm%s(\"%s\"",
			s->bits.asm_args->is_volatile ? " volatile" : "",
			s->bits.asm_args->cmd);

	if(s->bits.asm_args->extended){
		style_asm_params(s->bits.asm_args->params, 1);
		style_asm_params(s->bits.asm_args->params, 0);
	}

	stylef(")\n");
}
