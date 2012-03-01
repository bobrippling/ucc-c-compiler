#ifndef CC1_H
#define CC1_H

#define ASM_INLINE_FNAME "__asm__"

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
	WARN_ARG_MISMATCH     = 1 << 0,
	WARN_ARRAY_COMMA      = 1 << 1,
	WARN_ASSIGN_MISMATCH  = 1 << 2,
	WARN_COMPARE_MISMATCH = 1 << 3,
	WARN_RETURN_TYPE      = 1 << 4,
	WARN_SIGN_COMPARE     = 1 << 5,
	WARN_EXTERN_ASSUME    = 1 << 6,
	WARN_IMPLICIT_FUNC    = 1 << 7,
	WARN_IMPLICIT_INT     = 1 << 8,
	WARN_VOID_ARITH       = 1 << 9,
	WARN_OPT_POSSIBLE     = 1 << 20,
	WARN_SWITCH_ENUM      = 1 << 21,
	WARN_ENUM_CMP         = 1 << 22,
	WARN_INCOMPLETE_USE   = 1 << 23,
	WARN_UNUSED_EXPR      = 1 << 24,

	/* TODO */
	WARN_FORMAT           = 1 << 10,
	WARN_INT_TO_PTR       = 1 << 11,
	WARN_PTR_ARITH        = 1 << 12,
	WARN_SHADOW           = 1 << 13,
	WARN_UNINITIALISED    = 1 << 14,
	WARN_UNUSED_PARAM     = 1 << 15,
	WARN_UNUSED_VAL       = 1 << 16,
	WARN_UNUSED_VAR       = 1 << 17,
	WARN_ARRAY_BOUNDS     = 1 << 18,
	WARN_IDENT_TYPEDEF    = 1 << 19,
};

enum fopt
{
	FOPT_NONE            = 0,
	FOPT_ENABLE_ASM      = 1 << 0,
	FOPT_STRICT_TYPES    = 1 << 1,
	FOPT_CONST_FOLD      = 1 << 2,
	FOPT_ENGLISH         = 1 << 3,
	FOPT_DECL_PTR_TREE   = 1 << 4,
};

extern enum fopt fopt_mode;

void cc1_warn_atv(struct where *where, int die, enum warning w, const char *fmt, va_list l);
void cc1_warn_at( struct where *where, int die, enum warning w, const char *fmt, ...);

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

#endif
