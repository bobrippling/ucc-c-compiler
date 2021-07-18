#ifndef FORMAT_CHK_H
#define FORMAT_CHK_H

void format_check_call(type *fnty, expr **args, const int variadic);

void format_check_decl(decl *d, attribute *da);

#endif
