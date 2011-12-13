#ifndef CC1_H
#define CC1_H

enum section_type
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	NUM_SECTIONS
};

enum warning
{
	WARN_NONE             = 0,
	WARN_ARG_MISMATCH     = 1 << 1,
	WARN_ARRAY_COMMA      = 1 << 2,
	WARN_ASSIGN_MISMATCH  = 1 << 3,
	WARN_RETURN_TYPE      = 1 << 4,
	WARN_SIGN_COMPARE     = 1 << 5,
	WARN_EXTERN_ASSUME    = 1 << 6,
	WARN_IMPLICIT_FUNC    = 1 << 7,
	WARN_IMPLICIT_INT     = 1 << 8,
	WARN_VOID_ARITH       = 1 << 9,
};

void cc1_warn_at(struct where *where, int die, enum warning w, const char *fmt, ...);

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

#endif
