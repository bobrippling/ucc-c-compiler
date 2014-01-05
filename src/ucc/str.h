#ifndef STR_H
#define STR_H

char **strsplit(const char *, const char *sep);

/* mutates each argument */
char **strprepend(const char *j, char **args);

#endif
