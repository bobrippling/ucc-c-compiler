#include <stddef.h>

#include "sym.h"
#include "expr.h"
#include "sequence.h"
#include "warn.h"

#include "ops/expr_identifier.h"
#include "ops/expr_cast.h"

static sym *resolve(expr *e, sym *sym)
{
	decl *d;

	if(sym)
		return sym;

	if(expr_kind(e, cast)
	&& expr_cast_is_lval2rval(e)
	&& (d = expr_to_declref(expr_cast_child(e), NULL))
	&& d->sym)
	{
		return d->sym;
	}

	if(expr_kind(e, identifier) && e->bits.ident.type == IDENT_NORM)
		return e->bits.ident.bits.ident.sym;

	return NULL;
}

static void set_sym_state(sym *sym, symtable *symtab, enum sym_rw state, const where *where)
{
	symtable_global *glob = symtab_global(symtab);
	void *ent = dynmap_get(struct sym *, void *, glob->unsequenced_syms, sym);
	enum sym_rw existing;

	if(!ent){
		if(!glob->unsequenced_syms)
			glob->unsequenced_syms = dynmap_new(struct sym *, NULL, sym_hash);

		dynmap_set(struct sym *, void *, glob->unsequenced_syms, sym, (void *)(intptr_t)state);
		return;
	}

	existing = (intptr_t)ent;

	cc1_warn_at(where, unsequenced_access,
			"unsequenced modification of \"%s\"",
			sym->decl->spel);
}

void sequence_read(expr *e, sym *sym, symtable *symtab)
{
	sym = resolve(e, sym);
	if(!sym)
		return;

	set_sym_state(sym, symtab, SYM_UNSEQUENCED_READ, &e->where);
}

void sequence_write(expr *e, sym *sym, symtable *symtab)
{
	sym = resolve(e, sym);
	if(!sym)
		return;

	set_sym_state(sym, symtab, SYM_UNSEQUENCED_WRITE, &e->where);
}

void sequence_point(symtable *symtab)
{
	symtable_global *glob = symtab_global(symtab);
	dynmap_clear(glob->unsequenced_syms);
}
