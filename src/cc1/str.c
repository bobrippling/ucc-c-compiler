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

void cstring_init_ascii(struct cstring *out, const char *start, size_t in_len)
{
	out->type = CSTRING_RAW;
	out->count = in_len + 1 /* for nul byte */;

	out->bits.ascii = umalloc(out->count);

	memcpy(out->bits.ascii, start, in_len);
	out->bits.ascii[in_len] = '\0';
}

struct cstring *cstring_new_raw_from_ascii(const char *start, const char *end)
{
	struct cstring *alloc = umalloc(sizeof *alloc);

	cstring_init_ascii(alloc, start, end - start + 1);

	return alloc;
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

void cstring_escape(struct cstring *cstr, int is_wide)
{
	struct cstring tmpout = { 0 };
	size_t i, iout;

	assert(cstr->type == CSTRING_RAW);

	if(is_wide){
		tmpout.bits.wides = umalloc(cstr->count * sizeof(tmpout.bits.wides[0]));
		tmpout.type = CSTRING_WIDE;
	}else{
		tmpout.bits.ascii = umalloc(cstr->count);
		tmpout.type = CSTRING_ASCII;
	}

	/* "parse" into another string */
	for(i = iout = 0; i < cstr->count; i++){
		unsigned add;

		if(cstr->bits.ascii[i] == '\\'){
			extern int parse_had_error;
			where loc;
			char *end;
			int warn, err;

			/* assumes we're called immediately after tokenisation of string */
			where_cc1_current(&loc);
			loc.chr += i + 1;

			add = escape_char_1(&cstr->bits.ascii[i + 1], &end, is_wide, &warn, &err);

			UCC_ASSERT(end, "bad parse?");

			i = (end - cstr->bits.ascii) /*for the loop inc:*/- 1;

			switch(err){
				case 0:
					break;
				case EILSEQ:
					warn_at_print_error(&loc, "empty escape sequence");
					parse_had_error = 1;
					break;
				default:
					assert(0 && "unhandled escape error");
			}
			switch(warn){
				case 0:
					break;
				case ERANGE:
					warn_at_print_error(&loc, "escape sequence out of range");
					parse_had_error = 1;
					break;
				case EINVAL:
					warn_at_print_error(&loc, "invalid escape character");
					parse_had_error = 1;
					break;
				default:
					assert(0 && "unhandled escape warning");
			}

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

void cstring_free(struct cstring *cstr)
{
	if(cstr->type == CSTRING_WIDE)
		free(cstr->bits.wides);
	else
		free(cstr->bits.ascii);

	free(cstr);
}

int cstring_eq(const struct cstring *a, const struct cstring *b)
{
	size_t sz;
	void *ap, *bp;

	if(a->type != b->type)
		return 0;
	if(a->count != b->count)
		return 0;

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

	return !memcmp(ap, bp, sz * a->count);

}

unsigned cstring_hash(const struct cstring *cstr)
{
	unsigned hash = 5381;
	size_t i;

	for(i = 0; i < cstr->count; i++)
		hash = ((hash << 5) + hash) + cstring_char_at(cstr, i);

	return hash;
}

static void cstring_widen(struct cstring *cstr)
{
	int *wides = umalloc(sizeof(*wides) * cstr->count);
	size_t i;

	assert(cstr->type == CSTRING_RAW || cstr->type == CSTRING_ASCII);

	for(i = 0; i < cstr->count; i++)
		wides[i] = cstr->bits.ascii[i];

	free(cstr->bits.ascii);
	cstr->bits.wides = wides;
	cstr->type = CSTRING_WIDE;
}

void cstring_append(struct cstring *out, struct cstring *addend)
{
	if(addend->type != out->type){
		assert(addend->type == CSTRING_WIDE || out->type == CSTRING_WIDE);

		if(addend->type == CSTRING_WIDE)
			cstring_widen(out);
		else
			cstring_widen(addend);
	}

	out->count += addend->count /* ignore trailing \0 on ours */ - 1;

	if(out->type == CSTRING_WIDE){
		out->bits.wides = urealloc1(
				out->bits.wides,
				sizeof(out->bits.wides[0]) * out->count);

		memcpy(
				out->bits.wides + out->count - addend->count,
				addend->bits.wides,
				sizeof(addend->bits.wides[0]) * addend->count);

	}else{
		out->bits.ascii = urealloc1(out->bits.ascii, out->count);

		memcpy(
				out->bits.ascii + out->count - addend->count,
				addend->bits.ascii,
				addend->count);
	}
}

char *str_add_escape(struct cstring *cstr)
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

int literal_print(FILE *f, struct cstring *cstr)
{
	char *literal = str_add_escape(cstr);
	int r = fprintf(f, "%s", literal);
	free(literal);
	return r;
}
