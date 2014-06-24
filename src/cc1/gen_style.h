#ifndef GEN_STYLE_H
#define GEN_STYLE_H

#include "decl_init.h"

void stylef(const char *, ...) ucc_printflike(1, 2);

void gen_style_dinit(decl_init *);
void gen_style_decl(decl *);
void gen_style(symtable_global *);

#endif
