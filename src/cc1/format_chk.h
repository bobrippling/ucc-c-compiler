#ifndef FORMAT_CHK_H
#define FORMAT_CHK_H

void format_check_call(
		where *w, type *ref,
		expr **args, const int variadic);

void format_check_decl(decl *d, decl_attr *da);

#endif
