#include "ops.h"
#include "stmt_label.h"
#include "../label.h"
#include "../out/lbl.h"

const char *str_stmt_label()
{
	return "label";
}

void fold_stmt_label(stmt *s)
{
	label *l = symtab_label_find_or_new(
			s->symtab, s->bits.lbl.spel, &s->where);

	/* update its where */
	l->pw = &s->where;

	if(l->complete)
		die_at(&s->where, "duplicate label '%s'", s->bits.lbl.spel);
	else
		l->complete = 1;

	s->bits.lbl.label = l;

	l->unused = s->bits.lbl.unused;

	fold_stmt(s->lhs); /* compound */
}

void gen_stmt_label(stmt *s)
{
	out_label(s->bits.lbl.label->mangled);
	gen_stmt(s->lhs); /* the code-part of the compound statement */
}

void style_stmt_label(stmt *s)
{
	stylef("\n%s: ", s->bits.lbl.spel);
	gen_stmt(s->lhs);
}

int label_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void init_stmt_label(stmt *s)
{
	s->f_passable = label_passable;
}
