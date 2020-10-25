#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/escape.h"
#include "str.h"
#include "macros.h"
#include "cc1_where.h"
#include "warn.h"
#include "parse_fold_error.h"

void cstring_init(struct cstring *out, enum cstring_type t, const char *start, size_t len, int include_nul)
{
	out->type = t;
	out->count = len + !!include_nul;

	out->bits.ascii = umalloc(len + 1);
	memcpy(out->bits.ascii, start, len);
	out->bits.ascii[len] = '\0';
}

struct cstring *cstring_new(enum cstring_type t, const char *start, size_t len, int include_nul)
{
	struct cstring *cstr = umalloc(sizeof *cstr);

	cstring_init(cstr, t, start, len, include_nul);

	return cstr;
}

int cstring_char_at(const struct cstring *cstr, size_t i)
{
	if(cstr->type == CSTRING_WIDE)
		return cstr->bits.wides[i];
	else
		return cstr->bits.ascii[i];
}

static void cstring_char_set(struct cstring *cstr, size_t i, int to)
{
	if(cstr->type == CSTRING_WIDE)
		cstr->bits.wides[i] = to;
	else
		cstr->bits.ascii[i] = to;
}

static void cstring_widen(struct cstring *cstr)
{
	int *wides = umalloc(sizeof(*wides) * (cstr->count + 1));
	size_t i;

	assert(cstr->type == CSTRING_RAW || cstr->type == CSTRING_ASCII);

	for(i = 0; i < cstr->count + 1; i++)
		wides[i] = cstr->bits.ascii[i];

	free(cstr->bits.ascii);
	cstr->bits.wides = wides;
	cstr->type = CSTRING_WIDE;
}

static void cstring_asciify(struct cstring *cstr)
{
	char *ascii = umalloc(cstr->count + 1);
	size_t i;

	assert(cstr->type == CSTRING_WIDE);

	for(i = 0; i < cstr->count + 1; i++)
		ascii[i] = cstr->bits.wides[i];

	free(cstr->bits.wides);
	cstr->bits.ascii = ascii;
	cstr->type = CSTRING_ASCII;
}

void cstring_escape(
		struct cstring *cstr, int is_wide,
		void handle_escape_warn_err(int w, int e, int escape_offset, void *),
		void *ctx)
{
	struct cstring tmpout = { 0 };
	size_t i, iout;

	assert(cstr->type == CSTRING_RAW);

	if(is_wide){
		tmpout.bits.wides = umalloc((cstr->count + 1) * sizeof(tmpout.bits.wides[0]));
		tmpout.type = CSTRING_WIDE;
	}else{
		tmpout.bits.ascii = umalloc(cstr->count + 1);
		tmpout.type = CSTRING_ASCII;
	}

	/* "parse" into another string */
	for(i = iout = 0; i < cstr->count; i++){
		unsigned add;

		if(cstr->bits.ascii[i] == '\\'){
			int warn, err;
			const int escape_loc = i + 1;
			char *end;

			warn = err = 0;
			add = escape_char_1(&cstr->bits.ascii[i + 1], &end, is_wide, &warn, &err);

			UCC_ASSERT(end, "bad parse?");

			i = (end - cstr->bits.ascii) /*for the loop inc:*/- 1;

			handle_escape_warn_err(warn, err, escape_loc, ctx);
		}else{
			add = cstr->bits.ascii[i];
		}

		cstring_char_set(&tmpout, iout++, add);

		assert(iout <= cstr->count);
	}

	free(cstr->bits.ascii);

	if(is_wide)
		cstr->bits.wides = tmpout.bits.wides;
	else
		cstr->bits.ascii = tmpout.bits.ascii;
	cstr->type = tmpout.type;

	cstr->count = iout;
}

char *cstring_detach(struct cstring *cstr)
{
	char *r = cstr->bits.ascii;
	cstr->bits.ascii = NULL;

	assert(cstr->type != CSTRING_WIDE);

	cstring_free(cstr);

	return r;
}

char *cstring_converting_detach(struct cstring *cstr)
{
	switch(cstr->type){
		case CSTRING_WIDE:
			cstring_asciify(cstr);
			/* fallthrough */
		default:
			return cstring_detach(cstr);
	}
}

void cstring_deinit(struct cstring *cstr)
{
	if(cstr->type == CSTRING_WIDE)
		free(cstr->bits.wides);
	else
		free(cstr->bits.ascii);
}

void cstring_free(struct cstring *cstr)
{
	cstring_deinit(cstr);

	free(cstr);
}

int cstring_eq(const struct cstring *a, const struct cstring *b)
{
	enum { EQUAL, NOT_EQUAL };
	size_t sz;
	void *ap, *bp;

	if(a->type != b->type)
		return NOT_EQUAL;
	if(a->count != b->count)
		return NOT_EQUAL;

	switch(a->type){
		case CSTRING_WIDE:
			sz = sizeof(a->bits.wides[0]);
			ap = a->bits.wides;
			bp = b->bits.wides;
			break;
		case CSTRING_ASCII:
		case CSTRING_RAW:
			sz = 1;
			ap = a->bits.ascii;
			bp = b->bits.ascii;
			break;
	}

	return memcmp(ap, bp, sz * a->count);

}

unsigned cstring_hash(const struct cstring *cstr)
{
	unsigned hash = 5381;
	size_t i;

	for(i = 0; i < cstr->count; i++)
		hash = ((hash << 5) + hash) + cstring_char_at(cstr, i);

	return hash;
}

void cstring_append(struct cstring *out, struct cstring *addend)
{
	/* assumes if `out` ends with a nul, we remove it and maintain the nul-ness later */
	const size_t orig_out_count = out->count;
	int last_is_nul;

	if(out->type == CSTRING_WIDE){
		last_is_nul = out->bits.wides[orig_out_count - 1] == '\0';
	}else{
		last_is_nul = out->bits.ascii[orig_out_count - 1] == '\0';
	}

	if(addend->type != out->type){
		assert(addend->type == CSTRING_WIDE || out->type == CSTRING_WIDE);

		if(addend->type == CSTRING_WIDE)
			cstring_widen(out);
		else
			cstring_widen(addend);
	}

	out->count += addend->count;

	if(out->type == CSTRING_WIDE){
		out->bits.wides = urealloc1(
				out->bits.wides,
				sizeof(out->bits.wides[0]) * (out->count + 1));

		memcpy(
				out->bits.wides + out->count - addend->count - !!last_is_nul,
				addend->bits.wides,
				sizeof(addend->bits.wides[0]) * (addend->count + 1));

	}else{
		out->bits.ascii = urealloc1(out->bits.ascii, out->count + 1);

		memcpy(
				out->bits.ascii + out->count - addend->count - !!last_is_nul,
				addend->bits.ascii,
				addend->count + 1);
	}

	if(last_is_nul)
		out->count--;
}

char *str_add_escape(const struct cstring *cstr)
{
	size_t nlen = 0, i;
	char *new, *p;

	for(i = 0; i < cstr->count; i++){
		int v = cstring_char_at(cstr, i);

		if(v == '\\' || v == '"')
			nlen += 3;
		else if(!isprint(v))
			nlen += 4;
		else
			nlen++;
	}

	p = new = umalloc(nlen + 1);

	for(i = 0; i < cstr->count; i++){
		int v = cstring_char_at(cstr, i);

		if(v == '\\' || v == '"'){
			*p++ = '\\';
			*p++ = v;
		}else if(!isprint(v)){
			/* cast to unsigned char so we don't try printing
			 * some massive sign extended negative number */
			int n = sprintf(p, "\\%03o", (unsigned char)v);
			UCC_ASSERT(n <= 4, "sprintf(octal), length %d > 4", n);
			p += n;
		}else{
			*p++ = v;
		}
	}

	return new;
}

int literal_print(FILE *f, const struct cstring *cstr)
{
	char *literal = str_add_escape(cstr);
	int r = fprintf(f, "%s", literal);
	free(literal);
	return r;
}
