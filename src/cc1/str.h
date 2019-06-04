#ifndef STR_H
#define STR_H

#include "../util/unicode.h" /* char_type */

struct cstring
{
	union {
		char *u8;
		short *u16;
		int *u32;
	} bits;

	size_t count;

	enum cstring_type
	{
		CSTRING_RAW, /* .bits.ascii active, but not escaped */
		CSTRING_u8, /* used in ordered comparisons */
		CSTRING_u16,
		CSTRING_u32,
		CSTRING_WIDE,
	} type;
};

struct cstring *cstring_new(enum cstring_type, const char *start, size_t, int include_nul);
void cstring_init(struct cstring *, enum cstring_type, const char *start, size_t, int include_nul);

int cstring_char_at(const struct cstring *, size_t);

void cstring_escape(
		struct cstring *cstr, enum char_type char_type,
		void handle_escape_warn_err(int w, int e, int escape_offset, void *),
		void *ctx);

void cstring_append(struct cstring *, struct cstring *);

int cstring_eq(const struct cstring *, const struct cstring *);
unsigned cstring_hash(const struct cstring *);

void cstring_free(struct cstring *);
void cstring_deinit(struct cstring *);
char *cstring_detach(struct cstring *);
char *cstring_converting_detach(struct cstring *);

char *str_add_escape(const struct cstring *);
int literal_print(FILE *f, const struct cstring *);

#endif
