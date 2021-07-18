#ifndef BUILTIN_VA_H
#define BUILTIN_VA_H

expr *parse_va_start(const char *ident, symtable *scope);
expr *parse_va_arg(const char *ident, symtable *scope);
expr *parse_va_end(const char *ident, symtable *scope);
expr *parse_va_copy(const char *ident, symtable *scope);

#endif
