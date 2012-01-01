#ifndef MACRO_H
#define MACRO_H

void macro_add_dir(const char *);
void macro_add(const char *nam, const char *val);

void filter_macro(char **line);
void handle_macro(char *line);
void macro_finish(void);

#endif
