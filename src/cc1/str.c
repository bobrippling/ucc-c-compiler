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
#include "../util/unicode.h"

#include "str.h"
#include "macros.h"
#include "cc1_where.h"
#include "warn.h"
#include "parse_fold_error.h"

static void cstring_init_empty(struct cstring *out, enum cstring_type t, size_t len, int include_nul)
{
	out->type = t;
	out->count = len + !!include_nul;
}

void cstring_init(struct cstring *out, enum cstring_type t, const char *start, size_t len, int include_nul)
{
	cstring_init_empty(out, t, len, include_nul);

	out->bits.u8 = umalloc(len + 1);
	memcpy(out->bits.u8, start, len);
	out->bits.u8[len] = '\0';
}

struct cstring *cstring_new(enum cstring_type t, const char *start, size_t len, int include_nul)
{
	struct cstring *cstr = umalloc(sizeof *cstr);

	cstring_init(cstr, t, start, len, include_nul);

	return cstr;
}

int cstring_char_at(const struct cstring *cstr, size_t i)
{
	switch(cstr->type){
		case CSTRING_RAW:
			break;

		case CSTRING_u8:
			return cstr->bits.u8[i];

		case CSTRING_u16:
			return cstr->bits.u16[i];

		case CSTRING_u32:
		case CSTRING_WIDE:
			return cstr->bits.u32[i];
	}

	assert(0 && "unreachable");
}

static void cstring_char_set(struct cstring *cstr, size_t i, int to)
{
	switch(cstr->type){
		case CSTRING_RAW:
			assert(0 && "unreachable");

		case CSTRING_u8:
			cstr->bits.u8[i] = to;
			break;

		case CSTRING_u16:
			cstr->bits.u16[i] = to;
			break;

		case CSTRING_u32:
		case CSTRING_WIDE:
			cstr->bits.u32[i] = to;
			break;
	}
}

static void cstring_widen(struct cstring *cstr, enum cstring_type to)
{
	struct cstring new;
	size_t i;

	cstring_init_empty(&new, to, 0, 0);

	switch(to){
		case CSTRING_RAW:
		case CSTRING_u8:
			assert(0 && "unreachable");

		case CSTRING_u16:
			new.bits.u16 = umalloc((cstr->count + 1) * sizeof(new.bits.u16[0]));
			break;
		case CSTRING_u32:
		case CSTRING_WIDE:
			new.bits.u32 = umalloc((cstr->count + 1) * sizeof(new.bits.u32[0]));
			break;
	}

	for(i = 0; i < cstr->count + 1; i++)
		cstring_char_set(&new, i, cstring_char_at(cstr, i));

	cstring_deinit(cstr);
	memcpy(cstr, &new, sizeof(*cstr));
}

static void cstring_asciify(struct cstring *cstr)
{
	char *ascii = umalloc(cstr->count + 1);
	size_t i;

	for(i = 0; i < cstr->count + 1; i++)
		ascii[i] = cstring_char_at(cstr, i);

	cstring_deinit(cstr);
	cstr->bits.u8 = ascii;
	cstr->type = CSTRING_u8;
}

void cstring_escape(
		struct cstring *cstr, enum char_type char_type,
		void handle_escape_warn_err(int w, int e, int escape_offset, void *),
		void *ctx)
{
	struct cstring tmpout = { 0 };
	size_t i, iout;

	assert(cstr->type == CSTRING_RAW);

	switch(char_type){
		case UNICODE_NO:
		case UNICODE_u8:
			tmpout.bits.u8 = umalloc(cstr->count + 1);
			tmpout.type = CSTRING_u8;
			break;

		case UNICODE_u16:
			tmpout.bits.u32 = umalloc((cstr->count + 1) * sizeof(tmpout.bits.u16[0]));
			tmpout.type = CSTRING_u16;
			break;

		case UNICODE_U32:
		case UNICODE_L:
			tmpout.bits.u32 = umalloc((cstr->count + 1) * sizeof(tmpout.bits.u32[0]));
			tmpout.type = CSTRING_u32;
			break;
	}

	/* "parse" into another string */
	for(i = iout = 0; i < cstr->count; i++){
		unsigned add;

		if(cstr->bits.u8[i] == '\\'){
			int warn, err;
			const int escape_loc = i + 1;
			char *end;

			warn = err = 0;
			add = escape_char_1(&cstr->bits.u8[i + 1], &end, char_type > UNICODE_u8, &warn, &err);

			UCC_ASSERT(end, "bad parse?");

			i = (end - cstr->bits.u8) /*for the loop inc:*/- 1;

			handle_escape_warn_err(warn, err, escape_loc, ctx);
		}else{
			add = cstr->bits.u8[i];
		}

		cstring_char_set(&tmpout, iout++, add);

		assert(iout <= cstr->count);
	}

	cstring_deinit(cstr);

	memcpy(&cstr->bits, &tmpout.bits, sizeof(cstr->bits));
	cstr->type = tmpout.type;

	cstr->count = iout;
}

char *cstring_detach(struct cstring *cstr)
{
	char *r = cstr->bits.u8;
	cstr->bits.u8 = NULL;

	assert(cstr->type != CSTRING_WIDE);

	cstring_free(cstr);

	return r;
}

char *cstring_converting_detach(struct cstring *cstr)
{
	if(cstr->type > CSTRING_u8)
		cstring_asciify(cstr);

	return cstring_detach(cstr);
}

void cstring_deinit(struct cstring *cstr)
{
	switch(cstr->type){
		case CSTRING_RAW:
		case CSTRING_u8:
			free(cstr->bits.u8);
			break;

		case CSTRING_u16:
			free(cstr->bits.u16);
			break;

		case CSTRING_u32:
		case CSTRING_WIDE:
			free(cstr->bits.u32);
			break;
	}
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
		case CSTRING_RAW:
		case CSTRING_u8:
			sz = 1;
			ap = a->bits.u8;
			bp = b->bits.u8;
			break;

		case CSTRING_u16:
			sz = sizeof(a->bits.u16[0]);
			ap = a->bits.u16;
			bp = b->bits.u16;
			break;

		case CSTRING_u32:
		case CSTRING_WIDE:
			sz = sizeof(a->bits.u32[0]);
			ap = a->bits.u32;
			bp = b->bits.u32;
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

	switch(out->type){
		case CSTRING_RAW:
			assert(0 && "unreachable");
		case CSTRING_u8:
			last_is_nul = out->bits.u8[orig_out_count - 1] == '\0';
			break;
		case CSTRING_u16:
			last_is_nul = out->bits.u16[orig_out_count - 1] == '\0';
			break;
		case CSTRING_u32:
		case CSTRING_WIDE:
			last_is_nul = out->bits.u32[orig_out_count - 1] == '\0';
			break;
	}

	if(addend->type != out->type){
		assert(addend->type != CSTRING_RAW && out->type != CSTRING_RAW);

		if(addend->type < out->type)
			cstring_widen(addend, out->type);
		else
			cstring_widen(out, addend->type);
	}

	out->count += addend->count;

	switch(out->type){
		case CSTRING_RAW:
			assert(0 && "unreachable");

		case CSTRING_u8:
			out->bits.u8 = urealloc1(out->bits.u8, out->count + 1);

			memcpy(
					out->bits.u8 + out->count - addend->count - !!last_is_nul,
					addend->bits.u8,
					addend->count + 1);
			break;

		case CSTRING_u16:
			out->bits.u16 = urealloc1(
					out->bits.u16,
					sizeof(out->bits.u16[0]) * (out->count + 1));

			memcpy(
					out->bits.u16 + out->count - addend->count - !!last_is_nul,
					addend->bits.u16,
					sizeof(addend->bits.u16[0]) * (addend->count + 1));
			break;

		case CSTRING_u32:
		case CSTRING_WIDE:
			out->bits.u32 = urealloc1(
					out->bits.u32,
					sizeof(out->bits.u32[0]) * (out->count + 1));

			memcpy(
					out->bits.u32 + out->count - addend->count - !!last_is_nul,
					addend->bits.u32,
					sizeof(addend->bits.u32[0]) * (addend->count + 1));
			break;
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
