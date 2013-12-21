#ifndef STR_H
#define STR_H

void escape_string(char *str, size_t *len);

char *str_add_escape(const char *s, const size_t len);
int literal_print(FILE *f, const char *s, size_t len);

#endif
