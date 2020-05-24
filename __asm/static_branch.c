#define add_random_kstack_offset() do {					\
	if (static_branch_maybe(CONFIG_RANDOMIZE_KSTACK_OFFSET_DEFAULT,	\
				&randomize_kstack_offset)) {		\
		u32 offset = this_cpu_read(kstack_offset);		\
		char *ptr = __builtin_alloca(offset & 0x3FF);		\
		asm volatile("" : "=m"(*ptr));				\
	}								\
} while (0)

#define static_branch_maybe(...) x
int x;
typedef unsigned u32;
int kstack_offset;
u32 this_cpu_read(int);

main() {
	add_random_kstack_offset();
}

// --------------------------

// see https://gcc.gnu.org/pipermail/gcc-help/2019-June/135742.html

struct bug_entry {
	signed int      bug_addr_disp;
	signed int      file_disp;
	unsigned short  line;
	unsigned short  flags;
};

#define _BUG_FLAGS(ins, flags)             \
do {                                       \
	asm volatile("1:\t" ins "\n"             \
			".pushsection __bug_table,\"aw\"\n"  \
			"2:\t 1b - 2b \t# bug_addr_disp\n"   \
			"\t %c0 - 2b \t# file_disp\n"        \
			"\t.word %c1"        "\t# line\n"    \
			"\t.word %c2"        "\t# flags\n"   \
			"\t.org 2b+%c3\n"                    \
			".popsection"                        \
			: : "i" (__FILE__), "i" (__LINE__),  \
			"i" (flags),                         \
			"i" (sizeof(struct bug_entry)));     \
} while (0)

/*
 * There's a bit of magic involved here.
 *
 * First, unlike the bug table entries, we need to define an object in
 * assembly which we can reference from C code (for use by the
 * DYNAMIC_DEBUG_BRANCH macro), but we don't want 'name' to have
 * external linkage (that would require use of globally unique
 * identifiers, which we can't guarantee). Fortunately, the "extern"
 * keyword just tells the compiler that _somebody_ provides that
 * symbol - usually that somebody is the linker, but in this case it's
 * the assembler, and since we do not do .globl name, the symbol gets
 * internal linkage.
 *
 * So far so good. The next problem is that there's no scope in
 * assembly, so the identifier 'name' has to be unique within each
 * translation unit - otherwise all uses of that identifier end up
 * referring to the same struct _ddebug instance. pr_debug and friends
 * do this by use of indirection and __UNIQUE_ID(), and new users of
 * DEFINE_DYNAMIC_DEBUG_METADATA() should do something similar. We
 * need to catch cases where this is not done at build time.
 *
 * With assembly-level .ifndef we can ensure that we only define a
 * given identifier once, preventing "symbol 'foo' already defined"
 * errors. But we still need to detect and fail on multiple uses of
 * the same identifer. The simplest, and wrong, solution to that is to
 * add an .else .error branch to the .ifndef. The problem is that just
 * because the DEFINE_DYNAMIC_DEBUG_METADATA() macro is only expanded
 * once with a given identifier, the compiler may emit the assembly
 * code multiple times, e.g. if the macro appears in an inline
 * function. Now, in a normal case like
 *
 *   static inline get_next_id(void) { static int v; return ++v; }
 *
 * all inlined copies of get_next_id are _supposed_ to refer to the
 * same object 'v'. So we do need to allow this chunk of assembly to
 * appear multiple times with the same 'name', as long as they all
 * came from the same DEFINE_DYNAMIC_DEBUG_METADATA() instance. To do
 * that, we pass __COUNTER__ to the asm(), and set an assembler symbol
 * name.ddebug.once to that value when we first define 'name'. When we
 * meet a second attempt at defining 'name', we compare
 * name.ddebug.once to %6 and error out if they are different.
 */

#define DEFINE_DYNAMIC_DEBUG_METADATA(name, fmt)		\
	extern struct _ddebug name;				\
	asm volatile(".ifndef " __stringify(name) "\n"		\
		     ".pushsection __verbose,\"aw\"\n"		\
		     ".type "__stringify(name)", STT_OBJECT\n"	\
		     ".size "__stringify(name)", %c5\n"		\
		     "1:\n"					\
		     __stringify(name) ":\n"			\
		     "\t.int %c0 - 1b /* _ddebug::modname_disp */\n"\
		     "\t.int %c1 - 1b /* _ddebug::function_disp */\n"\
		     "\t.int %c2 - 1b /* _ddebug::filename_disp */\n"\
		     "\t.int %c3 - 1b /* _ddebug::format_disp */\n"\
		     "\t.int %c4      /* _ddebug::flags_lineno */\n"\
		     _DPRINTK_ASM_KEY_INIT			\
		     "\t.org 1b+%c5\n"				\
		     ".popsection\n"				\
		     ".set "__stringify(name)".ddebug.once, %c6\n"\
		     ".elseif "__stringify(name)".ddebug.once - %c6\n"\
		     ".line "__stringify(__LINE__) " - 1\n"            \
		     ".error \"'"__stringify(name)"' used as _ddebug identifier more than once\"\n" \
		     ".endif\n"					\
		     : : "i" (KBUILD_MODNAME), "i" (__func__),	\
		       "i" (__FILE__), "i" (fmt),		\
		       "i" (_DPRINTK_FLAGS_LINENO_INIT),	\
		       "i" (sizeof(struct _ddebug)), "i" (__COUNTER__))

//This is then used as in (a macro that expands to)

  do {
    DEFINE_DYNAMIC_DEBUG_METADATA(id1234, fmt);
    if (DYNAMIC_DEBUG_BRANCH(&id1234)) { /* [@] */
       do_something_with(&id1234, fmt, ...);
    }
  while (0)
