#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

type  *parse_type(void);
decl  *parse_decl(type *t, enum decl_mode mode);
decl  *parse_decl_single(enum decl_mode mode);
decl **parse_decls(const int can_default, const int accept_field_width);

#endif
