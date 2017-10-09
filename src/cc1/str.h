#ifndef STR_H
#define STR_H

struct cstring
{
	union {
		char *ascii;
		int *wides;
	} bits;

	size_t count;

	enum cstring_type
	{
		CSTRING_RAW, /* .bits.ascii active, but not escaped */
		CSTRING_ASCII,
		CSTRING_WIDE
	} type;
};

struct cstring *cstring_new(enum cstring_type, const char *start, size_t);
void cstring_init(struct cstring *, enum cstring_type, const char *start, size_t);

int cstring_char_at(const struct cstring *, size_t);

void cstring_escape(
		struct cstring *cstr, int is_wide,
		void handle_escape_warn_err(int w, int e, void *),
		void *ctx);

void cstring_append(struct cstring *, struct cstring *);

int cstring_eq(const struct cstring *, const struct cstring *);
unsigned cstring_hash(const struct cstring *);

void cstring_free(struct cstring *);
void cstring_deinit(struct cstring *);
char *cstring_detach(struct cstring *);

char *str_add_escape(struct cstring *);
int literal_print(FILE *f, struct cstring *);

#endif
