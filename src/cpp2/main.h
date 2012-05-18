#ifndef MAIN_H
#define MAIN_H

void dirname_push(char *d);
char *dirname_pop(void);

extern const char *current_fname;

extern char cpp_time[16], cpp_date[16];

extern int option_line_info;

#endif
