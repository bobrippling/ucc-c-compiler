#include <stdlib.h>
#include <assert.h>

#include "../../util/dynvec.h"

#include "ops.h"
#include "stmt_asm.h"
#include "../__asm.h"
#include "../out/__asm.h"

struct gen_asm_callback_ctx
{
	asm_param **params;
	struct constrained_val_array *outputs, *inputs;
};

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

			fold_check_expr(param->exp, FOLD_CHK_NO_BITFIELD_WARN, "asm output");

			fold_inc_writes_if_sym(param->exp, s->symtab);
		}
	}
}

static expr *cval_to_expr(
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
		expr *err_expr = cval_to_expr(
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

static void gen_stmt_asm_operand(
		out_ctx *octx, struct constrained_val *cval, const void *ctx)
{
	const struct gen_asm_callback_ctx *cb_ctx = ctx;
	expr *exp;

	assert(!cval->val && "already generated this value");

	exp = cval_to_expr(cb_ctx->params, cval, cb_ctx->outputs, cb_ctx->inputs);
	cval->val = gen_expr(exp, octx);
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

		if(param->is_output){
			new = dynvec_add(&outputs.arr, &outputs.n);
		}else{
			new = dynvec_add(&inputs.arr, &inputs.n);
		}

		new->ty = param->exp->tree_type;
		new->val = NULL;
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
		struct inline_asm_parameters asm_params;
		struct gen_asm_callback_ctx cb_ctx;

		cb_ctx.params = s->bits.asm_args->params;
		cb_ctx.outputs = &outputs;
		cb_ctx.inputs = &inputs;

		asm_params.format = s->bits.asm_args->cmd;
		asm_params.outputs = &outputs;
		asm_params.inputs = &inputs;
		asm_params.clobbers = s->bits.asm_args->clobbers;
		asm_params.where = &s->where;
		asm_params.fnty = fndecl->ref;
		asm_params.gen_callback = &gen_stmt_asm_operand;
		asm_params.gen_callback_ctx = &cb_ctx;

		out_inline_asm_ext_begin(octx, &asm_params, &state, &error);

		out_comment(octx, "### assignments to outputs");

		for(params = s->bits.asm_args->params, i = 0;
				params && *params;
				params++, i++)
		{
			asm_param *param = *params;
			if(!param->is_output)
				continue;

			if(!outputs.arr[i].val)
				gen_stmt_asm_operand(octx, &outputs.arr[i], &cb_ctx);

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
