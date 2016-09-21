#ifndef STR_H
#define STR_H

struct cstring
{
	union {
		char *ascii;
		int *wides;
	} bits;

	size_t count;

	enum {
		CSTRING_RAW, /* .bits.ascii active, but not escaped */
		CSTRING_ASCII,
		CSTRING_WIDE
	} type;
};

struct cstring *cstring_new_raw_from_ascii(const char *start, const char *end);
void cstring_init_ascii(struct cstring *, const char *, size_t);

int cstring_char_at(const struct cstring *, size_t);

void cstring_escape(
		struct cstring *cstr, int is_wide,
		void handle_escape_warn_err(int w, int e, void *),
		void *ctx);

void cstring_append(struct cstring *, struct cstring *);

int cstring_eq(const struct cstring *, const struct cstring *);
unsigned cstring_hash(const struct cstring *);

void cstring_free(struct cstring *);
char *cstring_detach(struct cstring *);

char *str_add_escape(struct cstring *);
int literal_print(FILE *f, struct cstring *);

#endif
