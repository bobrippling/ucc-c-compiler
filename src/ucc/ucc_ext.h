#ifndef UCC_EXT_H
#define UCC_EXT_H

char *actual_path(const char *prefix, const char *path);

void preproc( char *in,    char *out, char **args);
void compile( char *in,    char *out, char **args);
void assemble(char *in,    char *out, char **args);
void link(    char **objs, char *out, char **args);

#endif
