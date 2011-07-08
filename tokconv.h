#ifndef TOKCONV_H
#define TOKCONV_H

enum type curtok_to_type();
enum expr_op curtok_to_op();
void eat(enum token);
char *token_current_spel();
char *token_current_str();
int curtok_is_type();

#endif
