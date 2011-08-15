#ifndef TOKCONV_H
#define TOKCONV_H

enum type    curtok_to_type();
enum op_type curtok_to_op();

void eat(enum token);

int curtok_is_type();
int curtok_in_list(va_list l);

char *token_current_spel();
void token_get_current_str(char **ps, int *pl);

#endif
