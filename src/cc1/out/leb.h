#ifndef LEB_H
#define LEB_H

unsigned leb128_length(unsigned long long value, int is_signed);

/* returns length */
unsigned leb128_out(FILE *, unsigned long long, int sig);

#endif
