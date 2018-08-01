/*
 * NOTE: fopts that take a value,
 * e.g. -ferror-limit=...
 * are handled manually in cc1
 */

X("enable-asm", enable_asm)
X("const-fold", const_fold)
X("english", english)
X("show-line", show_line)
X("pic", pic)
ALIAS("PIC", pic)
X("plt", plt)
X("builtin", builtin)
X("ms-extensions", ms_extensions)
X("plan9-extensions", plan9_extensions)
X("leading-underscore", leading_underscore)
X("trapv", trapv)
X("track-initial-fname", track_initial_fnam)
X("freestanding", freestanding)
X("show-static-asserts", show_static_asserts)
X("verbose-asm", verbose_asm)
X("integral-float-load", integral_float_load)
X("symbol-arith", symbol_arith)

X("signed-char", signed_char)
INVERT("unsigned-char", signed_char)

/* tag_anon_struct_ext = fopt_ms_extensions | fopt_plan9_extensions */
#define FOPT_TAG_ANON_STRUCT_EXT(fopt) \
	((fopt)->ms_extensions || (fopt)->plan9_extensions)

X("cast-with-builtin-types", cast_w_builtin_types)
X("dump-type-tree", dump_type_tree)
X("asm", ext_keywords)
ALIAS("gnu-keywords", ext_keywords)
X("fold-const-vlas", fold_const_vlas)
X("show-warning-option", show_warning_option)
X("print-typedefs", print_typedefs)
X("print-aka", print_aka)
X("show-inlined", show_inlined)
X("inline-functions", inline_functions)
X("dump-bblocks", dump_basic_blocks)
X("dump-symtab", dump_symtab)
X("dump-init", dump_init)
X("common", common)
X("short-enums", short_enums)
X("thread-jumps", thread_jumps)
