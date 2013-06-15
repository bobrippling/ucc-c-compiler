#ifndef STR_H
#define STR_H

int  escape_char(int c);
void escape_string(char *str, int *len);

int literal_print(FILE *f, const char *s, int len);

enum base
{
	BIN, OCT, DEC, HEX
};

const char *base_to_str(enum base);

void char_seq_to_iv(char *s, numeric *iv, int *plen, enum base mode);

#define isoct(x) ('0' <= (x) && (x) < '8')

#endif
