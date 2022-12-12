#ifndef OUT_ASM_H
#define OUT_ASM_H

#include <stdio.h>

#include "section.h"

const char *asm_section_desc(enum section_builtin);

ucc_printflike(2, 3)
ucc_nonnull()
void asm_out_section(const struct section *, const char *fmt, ...);

ucc_nonnull()
void asm_out_sectionv(const struct section *, const char *fmt, va_list l);

void asm_switch_section(const struct section *);

int asm_section_empty(const struct section *);

ucc_nonnull()
void asm_out_align(const struct section *sec, unsigned align);
void asm_nam_begin3(const struct section *, const char *lbl, unsigned align);

#ifdef TYPE_H
void asm_out_fp(const struct section *, type *ty, floating_t f);
#endif

#ifdef STRINGS_H
void asm_declare_stringlit(const struct section *, const stringlit *);
#endif

#ifdef DECL_H
void asm_declare_decl_init(const struct section *, decl *); /* x: .qword ... */

void asm_declare_constructor(decl *d);
void asm_declare_destructor(decl *d);

void asm_predeclare_extern(const struct section *, decl *d);
void asm_predeclare_global(const struct section *, decl *d);
void asm_predeclare_weak(const struct section *, decl *d);
void asm_predeclare_used(const struct section *, decl *d);
void asm_predeclare_visibility(const struct section *, decl *d);
void asm_declare_alias(const struct section *, decl *d, decl *alias);
#endif

extern const struct asm_type_table
{
	int sz;
	const char *directive;
} asm_type_table[];
#define ASM_TABLE_LEN 4

#ifdef TYPE_H
int         asm_table_lookup(type *);
int         asm_type_size(type *);
const char *asm_type_directive(type *);
#endif

#endif
