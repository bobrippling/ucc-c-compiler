#ifndef GEN_STYLE_H
#define GEN_STYLE_H

#define stylef(...) fprintf(cc1_out, __VA_ARGS__)

void gen_style_dinit(decl_init *);
void gen_style_decl(decl *);
void gen_style(symtable_global *, const char *);

#endif
