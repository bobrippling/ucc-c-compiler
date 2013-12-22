#ifndef LEB_H
#define LEB_H

unsigned leb128_length(unsigned long long value, int is_signed);
void leb128_out(FILE *, unsigned long long, int sig);

#endif
