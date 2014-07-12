#ifndef CC1_H
#define CC1_H

#include "../util/std.h"

/*#define FANCY_STACK_INIT 1*/

enum warning
{
	WARN_NONE                     = 0,
	WARN_ARG_MISMATCH             = 1 << 0,
	WARN_ARRAY_COMMA              = 1 << 1,
	WARN_ASSIGN_MISMATCH          = 1 << 2,
	WARN_COMPARE_MISMATCH         = 1 << 3,
	WARN_RETURN_TYPE              = 1 << 4,
	WARN_SIGN_COMPARE             = 1 << 5,
	WARN_EXTERN_ASSUME            = 1 << 6,
	WARN_IMPLICIT_FUNC            = 1 << 7,
	WARN_IMPLICIT_INT             = 1 << 8,
	WARN_VOID_ARITH               = 1 << 9,
	WARN_MIXED_CODE_DECLS         = 1 << 10,
	WARN_TEST_BOOL                = 1 << 11,
	WARN_LOSS_PRECISION           = 1 << 12,

	WARN_OPT_POSSIBLE             = 1 << 13,
	WARN_SWITCH_ENUM              = 1 << 14,
	WARN_ENUM_CMP                 = 1 << 15,
	WARN_INCOMPLETE_USE           = 1 << 16,
	WARN_UNUSED_EXPR              = 1 << 17,
	WARN_TEST_ASSIGN              = 1 << 18,
	WARN_READ_BEFORE_WRITE        = 1 << 19,
	WARN_SYM_NEVER_WRITTEN        = 1 << 20,
	WARN_SYM_NEVER_READ           = 1 << 21,
	WARN_DEAD_CODE                = 1 << 22,
	WARN_PREDECL_ENUM             = 1 << 23,
	WARN_OMITTED_PARAM_TYPES      = 1 << 24,
	WARN_RETURN_UNDEF             = 1 << 25,
	WARN_PAD                      = 1 << 26,
	WARN_TENATIVE_INIT            = 1 << 27,

	WARN_SHADOW_LOCAL             = 1 << 28,
	WARN_SHADOW_GLOBAL            = 1 << 29,

	WARN_IMPLICIT_OLD_FUNC        = 1 << 30, /* int f(); */

	/* TODO */
	/*
	WARN_FORMAT                   = 1 << 23,
	WARN_INT_TO_PTR               = 1 << 24,
	WARN_PTR_ARITH                = 1 << 25,
	WARN_SHADOW                   = 1 << 26,
	WARN_UNINITIALISED            = 1 << 27,
	WARN_UNUSED_PARAM             = 1 << 28,
	WARN_UNUSED_VAL               = 1 << 29,
	WARN_UNUSED_VAR               = 1 << 30,
	WARN_ARRAY_BOUNDS             = 1 << 31,
	*/
};

enum fopt
{
	FOPT_NONE                  = 0,
	FOPT_ENABLE_ASM            = 1 << 0,
	FOPT_CONST_FOLD            = 1 << 1,
	FOPT_ENGLISH               = 1 << 2,
	FOPT_SHOW_LINE             = 1 << 3,
	FOPT_PIC                   = 1 << 4,
	FOPT_PIC_PCREL             = 1 << 5,
	FOPT_BUILTIN               = 1 << 6,
	FOPT_MS_EXTENSIONS         = 1 << 7,
	FOPT_PLAN9_EXTENSIONS      = 1 << 8,
	FOPT_TAG_ANON_STRUCT_EXT   = FOPT_MS_EXTENSIONS | FOPT_PLAN9_EXTENSIONS,
	FOPT_LEADING_UNDERSCORE    = 1 << 9,
	FOPT_TRAPV                 = 1 << 10,
	FOPT_TRACK_INITIAL_FNAM    = 1 << 11,
	FOPT_FREESTANDING          = 1 << 12,
	FOPT_SHOW_STATIC_ASSERTS   = 1 << 13,
	FOPT_VERBOSE_ASM           = 1 << 14,
	FOPT_INTEGRAL_FLOAT_LOAD   = 1 << 15,
	FOPT_SYMBOL_ARITH          = 1 << 16,
	FOPT_SIGNED_CHAR           = 1 << 17,
	FOPT_CAST_W_BUILTIN_TYPES  = 1 << 18,
	FOPT_DUMP_TYPE_TREE        = 1 << 19,
	FOPT_EXT_KEYWORDS          = 1 << 20, /* -fasm */
	FOPT_FOLD_CONST_VLAS       = 1 << 21,
};

enum mopt
{
	MOPT_32            = 1 << 0,
	MOPT_STACK_REALIGN = 1 << 1,
};
#define IS_32_BIT() (!!(mopt_mode & MOPT_32))

enum cc1_backend
{
	BACKEND_ASM,
	BACKEND_PRINT,
	BACKEND_STYLE,
};

extern enum fopt fopt_mode;
extern enum mopt mopt_mode;
extern enum cc1_backend cc1_backend;
extern enum warning warn_mode;

extern enum c_std cc1_std;
#define C99_LONGLONG() if(cc1_std < STD_C99) warn_at(NULL, "long long is a C99 feature")

void cc1_warn_atv(struct where *where, int die, enum warning w, const char *fmt, va_list l);
void cc1_warn_at( struct where *where, int die, enum warning w, const char *fmt, ...) ucc_printflike(4, 5);

extern int cc1_error_limit;

extern int cc1_mstack_align; /* 2^n */
extern int cc1_gdebug; /* -g */

extern char *cc1_first_fname;

#endif
