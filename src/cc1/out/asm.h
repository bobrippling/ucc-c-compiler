#ifndef OUT_ASM_H
#define OUT_ASM_H

#include <stdio.h>

#include "section.h"

const char *asm_section_desc(enum section_builtin);

/* this is temporary, until the __attribute__((section("..."))) branch
 * comes in, when its struct { builtin | name } will be used as a key,
 * instead of just a string */
enum section_builtin asm_builtin_section_from_str(const char *);

FILE *asm_section_file(const struct section *);

ucc_printflike(2, 3)
void asm_out_section(const struct section *, const char *fmt, ...);
void asm_out_sectionv(const struct section *, const char *fmt, va_list l);

int asm_section_empty(const struct section *);

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
void asm_predeclare_visibility(const struct section *, decl *d);
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
