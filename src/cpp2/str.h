#ifndef STR_H
#define STR_H

int  iswordpart(char c);
char *word_dup(const char *);
char *word_end(char *); /* word_end("hello=there") -> ptr to '=' */
char *word_find_any(char *);

char *str_quote(char *quoteme, int free_in);
char *str_join(char **, const char *with);
void  str_trim(char *);

char *str_replace(char *line, char *start, char *end, const char *replace);

char *word_replace(char *line, char *pos, size_t len, const char *replace);
char *word_find(   char *line, const char *word);

char *strchr_nest(char *start, char find);

#endif
