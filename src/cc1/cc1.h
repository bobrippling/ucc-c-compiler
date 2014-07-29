#ifndef CC1_H
#define CC1_H

#include "../util/std.h"

struct cc1_warning
{
	unsigned char arg_mismatch;
	unsigned char array_comma;
	unsigned char assign_mismatch;
	unsigned char compare_mismatch;
	unsigned char return_type;
	unsigned char sign_compare;
	unsigned char extern_assume;
	unsigned char implicit_func;
	unsigned char implicit_int;
	unsigned char void_arith;
	unsigned char mixed_code_decls;
	unsigned char test_bool;
	unsigned char loss_precision;

	unsigned char opt_possible;
	unsigned char switch_enum;
	unsigned char enum_cmp;
	unsigned char incomplete_use;
	unsigned char unused_expr;
	unsigned char test_assign;
	unsigned char read_before_write;
	unsigned char sym_never_written;
	unsigned char sym_never_read;
	unsigned char dead_code;
	unsigned char predecl_enum;
	unsigned char omitted_param_types;
	unsigned char return_undef;
	unsigned char pad;
	unsigned char tenative_init;

	unsigned char shadow_local;
	unsigned char shadow_global;

	unsigned char implicit_old_func;

	unsigned char format;
	unsigned char int_to_ptr;
	unsigned char ptr_arith;
	unsigned char shadow;
	unsigned char uninitialised;
	unsigned char unused_param;
	unsigned char unused_val;
	unsigned char unused_var;
	unsigned char array_bounds;

	unsigned char cast_qual;
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

extern struct cc1_warning cc1_warning;

extern enum c_std cc1_std;
#define C99_LONGLONG() if(cc1_std < STD_C99) warn_at(NULL, "long long is a C99 feature")

void cc1_warn_at(struct where *where, const char *fmt, ...) ucc_printflike(2, 3);
#define cc1_warn_at(loc, warn, ...) do{ \
		if(cc1_warning.warn) \
			cc1_warn_at(loc, __VA_ARGS__); \
	}while(0)

extern int cc1_error_limit;

extern int cc1_mstack_align; /* 2^n */
extern int cc1_gdebug; /* -g */

extern char *cc1_first_fname;

#endif
