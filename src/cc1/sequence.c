#include <stddef.h>
#include <assert.h>

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

static void set_sym_entry(symtable_global *symtabg, sym *sym, enum sym_rw new_state)
{
	if(!symtabg->unsequenced_syms)
		symtabg->unsequenced_syms = dynmap_new(struct sym *, NULL, sym_hash);

	dynmap_set(struct sym *, void *, symtabg->unsequenced_syms, sym, (void *)(intptr_t)new_state);
}

static void update_sym(sym *sym, symtable *symtab, enum sym_rw new_state, const where *where)
{
	symtable_global *glob = symtab_global(symtab);
	void *ent = dynmap_get(struct sym *, void *, glob->unsequenced_syms, sym);
	enum sym_rw existing;

	if(!ent){
		set_sym_entry(glob, sym, new_state);
		return;
	}

	existing = (intptr_t)ent;
	if(existing == SYM_UNSEQUENCED_READ){
		switch(new_state){
			case SYM_UNSEQUENCED_UNSET:
				assert(0 && "unreachable");
			case SYM_UNSEQUENCED_READ:
				return;
			case SYM_UNSEQUENCED_WRITE:
				/* read then write, unsequenced - change to the more extreme write state,
				 * and fall through to warning */
				set_sym_entry(glob, sym, SYM_UNSEQUENCED_WRITE);
				break;
		}
	}

	cc1_warn_at(where, unsequenced_access,
			"unsequenced modification of \"%s\"",
			sym->decl->spel);
}

void sequence_read(expr *e, sym *sym, symtable *symtab)
{
	sym = resolve(e, sym);
	if(!sym)
		return;

	update_sym(sym, symtab, SYM_UNSEQUENCED_READ, &e->where);
}

void sequence_write(expr *e, sym *sym, symtable *symtab)
{
	sym = resolve(e, sym);
	if(!sym)
		return;

	update_sym(sym, symtab, SYM_UNSEQUENCED_WRITE, &e->where);
}

void sequence_point(symtable *symtab)
{
	symtable_global *glob = symtab_global(symtab);
	dynmap_clear(glob->unsequenced_syms);
}

enum sym_rw sequence_state(expr *e, sym *sym, symtable *symtab)
{
	symtable_global *glob;
	void *ent;

	sym = resolve(e, sym);
	if(!sym)
		return SYM_UNSEQUENCED_UNSET;

	glob = symtab_global(symtab);
	ent = dynmap_get(struct sym *, void *, glob->unsequenced_syms, sym);

	if(!ent)
		return SYM_UNSEQUENCED_UNSET;
	return (intptr_t)ent;
}

void sequence_set_state(expr *e, sym *sym, symtable *symtab, enum sym_rw new_state)
{
	sym = resolve(e, sym);
	if(!sym)
		return;

	set_sym_entry(symtab_global(symtab), sym, new_state);
}
