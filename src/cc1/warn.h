#ifndef CC1_WARN_H
#define CC1_WARN_H

struct cc1_warning
{
	unsigned char arg_mismatch;
	unsigned char arith_fnptr;
	unsigned char arith_voidp;
	unsigned char array_comma;
	unsigned char array_oob;
	unsigned char asm_badchar;
	unsigned char assign_mismatch;
	unsigned char attr_badcleanup;
	unsigned char attr_format_baddecl;
	unsigned char attr_format_unknown;
	unsigned char attr_noderef;
	unsigned char attr_nonnull;
	unsigned char attr_nonnull_bad;
	unsigned char attr_nonnull_noargs;
	unsigned char attr_nonnull_noptrs;
	unsigned char attr_nonnull_nonptr;
	unsigned char attr_nonnull_oob;
	unsigned char attr_printf_bad;
	unsigned char attr_printf_toomany;
	unsigned char attr_printf_unknown;
	unsigned char attr_printf_voidp;
	unsigned char attr_section_badchar;
	unsigned char attr_sentinel;
	unsigned char attr_sentinel_nonvariadic;
	unsigned char attr_unknown;
	unsigned char attr_unused_used;
	unsigned char attr_unused_voidfn;
	unsigned char bad_funcqual;
	unsigned char bad_inline;
	unsigned char bad_restrict;
	unsigned char bitfield_boundary;
	unsigned char bitfield_onebit_int;
	unsigned char bitfield_trunc;
	unsigned char builtin_expect_nonconst;
	unsigned char builtin_memset_bad;
	unsigned char builtin_va_arg;
	unsigned char builtin_va_start;
	unsigned char c89_compound_literal;
	unsigned char c89_for_init;
	unsigned char c89_init_constexpr;
	unsigned char c89_parse_trailingcomma;
	unsigned char cast_qual;
	unsigned char compare_mismatch;
	unsigned char constant_large_unsigned;
	unsigned char constop_bad;
	unsigned char dead_code;
	unsigned char decl_nodecl;
	unsigned char empty_struct;
	unsigned char enum_cmp;
	unsigned char enum_mismatch;
	unsigned char enum_switch_bitmask;
	unsigned char enum_switch_imposter;
	unsigned char excess_init;
	unsigned char extern_assume;
	unsigned char extern_init;
	unsigned char flexarr_embed;
	unsigned char flexarr_init;
	unsigned char flexarr_only;
	unsigned char funcall_argcount;
	unsigned char gnu_addr_lbl;
	unsigned char gnu_alignof_expr;
	unsigned char gnu_empty_init;
	unsigned char gnu_expr_stmt;
	unsigned char gnu_typeof;
	unsigned char gnu_attribute;
	unsigned char gnu_init_array_range;
	unsigned char gnu_case_range;
	unsigned char gnu_gcc_compat;
	unsigned char ignored_attribute;
	unsigned char ignored_late_decl;
	unsigned char ignored_qualifiers;
	unsigned char implicit_func;
	unsigned char implicit_int;
	unsigned char implicit_old_func;
	unsigned char incomplete_use;
	unsigned char init_missing_braces;
	unsigned char init_missing_struct;
	unsigned char init_missing_struct_zero;
	unsigned char init_obj_discard;
	unsigned char init_overlong_strliteral;
	unsigned char init_override;
	unsigned char init_undesignated;
	unsigned char inline_builtin_frame_addr;
	unsigned char int_ptr_conv;
	unsigned char lbl_attr_unknown;
	unsigned char lbl_unused;
	unsigned char long_long;
	unsigned char loss_precision;
	unsigned char mismatch_conditional;
	unsigned char mismatch_ptr;
	unsigned char mismatching_types;
	unsigned char missing_empty_struct_brace_init;
	unsigned char mixed_code_decls;
	unsigned char multichar;
	unsigned char multichar_toolarge;
	unsigned char nonstd_arraysz;
	unsigned char nonstd_init;
	unsigned char omitted_param_types;
	unsigned char op_shift_bad;
	unsigned char overflow;
	unsigned char overlarge_enumerator_bitfield;
	unsigned char overlarge_enumerator_int;
	unsigned char pad;
	unsigned char parse_extra_semi;
	unsigned char parse_precedence;
	unsigned char predecl_enum;
	unsigned char private_struct;
	unsigned char pure_inline;
	unsigned char restrict_ptrs;
	unsigned char return_type;
	unsigned char return_undef;
	unsigned char return_void;
	unsigned char shadow_global;
	unsigned char shadow_local;
	unsigned char sign_compare;
	unsigned char signed_unsigned;
	unsigned char sizeof_decayed;
	unsigned char sizeof_ptr_div;
	unsigned char static_array_bad;
	unsigned char static_local_in_inline;
	unsigned char str_contain_nul;
	unsigned char struct_noinstance_anon;
	unsigned char struct_noinstance_qualified;
	unsigned char switch_enum;
	unsigned char sym_never_read;
	unsigned char sym_never_written;
	unsigned char tautologic_unsigned;
	unsigned char tenative_array_1elem;
	unsigned char tenative_init;
	unsigned char test_assign;
	unsigned char test_bool;
	unsigned char typedef_fnimpl;
	unsigned char typedef_redef;
	unsigned char undef_internal;
	unsigned char undef_strlitcmp;
	unsigned char uninitialised;
	unsigned char unnamed_struct_memb;
	unsigned char unused_comma;
	unsigned char unused_expr;
	unsigned char unused_param;
	unsigned char unused_val;
	unsigned char unused_var;
	unsigned char vla;
	unsigned char x__func__init;
	unsigned char x__func__outsidefn;
};

extern struct cc1_warning cc1_warning;

void cc1_warn_at_w(
		const struct where *where,
		unsigned char *pwarn,
		const char *fmt, ...)
	ucc_printflike(3, 4);

#define cc1_warn_at(loc, warn, ...) do{ \
		cc1_warn_at_w(loc, &cc1_warning.warn, __VA_ARGS__); \
	}while(0)

#endif
