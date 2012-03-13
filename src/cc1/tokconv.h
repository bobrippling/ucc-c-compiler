#ifndef TOKCONV_H
#define TOKCONV_H

enum type_primitive curtok_to_type_primitive(void);
enum type_spec      curtok_to_type_specifier(void);

void eat(enum token, const char *fnam, int line);

#define EAT(t)      eat((t), __FILE__, __LINE__)
#define accept(tok) ((tok) == curtok ? (EAT(tok), 1) : 0)

int curtok_is_type(void);
int curtok_is_type_specifier(void);
int curtok_in_list(va_list l);

char *token_current_spel(void);
char *token_current_spel_peek(void);
void token_get_current_str(char **ps, int *pl);

int curtok_is_augmented_assignment(void);

const char *token_to_str(enum token t);

op *op_from_token(enum token, expr *, expr *);

#endif
