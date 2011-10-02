#ifndef TOKCONV_H
#define TOKCONV_H

enum type_primitive curtok_to_type_primitive();
enum type_spec      curtok_to_type_specifier();
enum op_type        curtok_to_op();

void eat(enum token, const char *fnam, int line);

#define EAT(t) eat(t, __FILE__, __LINE__)

int curtok_is_type();
int curtok_is_type_specifier();
int curtok_is_type_prething();
int curtok_in_list(va_list l);

char *token_current_spel();
void token_get_current_str(char **ps, int *pl);

enum op_type curtok_to_augmented_op();
int          curtok_is_augmented_assignment();

const char *token_to_str(enum token t);

#endif
