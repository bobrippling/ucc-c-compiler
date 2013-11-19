#ifndef TOKCONV_H
#define TOKCONV_H

enum op_type        curtok_to_op(void);

enum type_primitive curtok_to_type_primitive(void);
enum type_qualifier curtok_to_type_qualifier(void);
enum decl_storage   curtok_to_decl_storage(void);

void eat( enum token t, const char *fnam, int line);
void eat2(enum token t, const char *fnam, int line, int die);
void uneat(enum token t);
int accept(enum token t);
int accept_where(enum token t, where *);

#define EAT(t)         eat( (t), __FILE__, __LINE__)
#define EAT_OR_DIE(t)  eat2((t), __FILE__, __LINE__, 1)

int curtok_is_type_primitive(void);
int curtok_is_type_qual(void);
int curtok_is_decl_store(void);

int curtok_in_list(va_list l);

void token_get_current_str(
		char **ps, int *pl, int *pwide, where *w);

enum op_type curtok_to_compound_op(void);
int          curtok_is_compound_assignment(void);

char *token_to_str(enum token t);
char *curtok_to_identifier(int *alloc); /* e.g. token_const -> "const" */

extern int parse_had_error;

#endif
