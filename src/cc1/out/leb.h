#ifndef LEB_H
#define LEB_H

unsigned leb128_length(unsigned long long value, int is_signed);

unsigned leb128_emit(
		unsigned long long v,
		int is_signed,
		void emit_byte(unsigned char, void *),
		void *ctx);

unsigned leb128_out(FILE *, unsigned long long, int sig);

#endif
