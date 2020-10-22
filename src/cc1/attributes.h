#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

/* name/spelling + name,
 * then an int describing whether the attribute
 * takes part in type propagation
 *
 * NAME(enum_identifier, typrop)
 * RENAME(str, enum_identifier, typrop)
 * ALIAS(str, enum_identifier)          // alias for a NAME()
 * COMPLEX_ALIAS(str, enum_identifier)  // str used, but needs manual parsing
 */

#define ATTRIBUTES    \
		NAME(format, 0)         \
		NAME(unused, 0)         \
		NAME(used, 0)           \
		NAME(warn_unused, 0)    \
		NAME(section, 0)        \
		NAME(enum_bitmask, 0)   \
		COMPLEX_ALIAS("flag_enum", enum_bitmask) /* clang */ \
		NAME(noreturn, 1)       \
		NAME(noderef, 1)        \
		NAME(nonnull, 1)        \
		NAME(returns_nonnull, 1)\
		NAME(packed, 1)         \
		NAME(sentinel, 0)       \
		NAME(aligned, 1)        \
		NAME(weak, 0)           \
		NAME(alias, 0)          \
		NAME(cleanup, 0)        \
		NAME(always_inline, 0)  \
		NAME(noinline, 0)       \
		NAME(constructor, 0)    \
		NAME(destructor, 0)     \
		NAME(visibility, 0)     \
		NAME(stack_protect, 0) /* gcc */ \
		NAME(no_stack_protector, 0) /* clang */ \
		NAME(no_sanitize, 0) \
		NAME(fallthrough, 0) \
		\
		/* like NAME() but the enumerator is different to the spel */ \
		RENAME("designated_init", desig_init, 0)     \
		RENAME("__ucc_debug", ucc_debug, 0) /* logs out a message when handled */ \
		RENAME("__cdecl", call_conv, 0)        \
		\
		/* an attribute under a different name */ \
		ALIAS("warn_unused_result", warn_unused) \
		\
		/* these need additional code to handle */ \
		COMPLEX_ALIAS("cdecl", call_conv)    \
		COMPLEX_ALIAS("stdcall", call_conv)  \
		COMPLEX_ALIAS("fastcall", call_conv) \
		/*COMPLEX_ALIAS("no_sanitize_address", no_sanitize)*/ \
		/*COMPLEX_ALIAS("no_address_safety_analysis", no_sanitize)*/ \
		/*COMPLEX_ALIAS("no_sanitize_thread", no_sanitize)*/ \
		COMPLEX_ALIAS("no_sanitize_undefined", no_sanitize)

#endif
