#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

/* name/spelling + name,
 * then an int describing whether the attribute
 * takes part in type propagation */

#define ATTRIBUTES    \
		NAME(format, 0)         \
		NAME(unused, 0)         \
		NAME(warn_unused, 0)    \
		NAME(section, 0)        \
		NAME(enum_bitmask, 0)   \
		NAME(noreturn, 1)       \
		NAME(noderef, 1)        \
		NAME(nonnull, 1)        \
		NAME(packed, 1)         \
		NAME(sentinel, 0)       \
		NAME(aligned, 1)        \
		NAME(weak, 0)           \
		NAME(cleanup, 0)        \
		NAME(always_inline, 0)  \
		NAME(noinline, 0)       \
		NAME(constructor, 0)    \
		NAME(destructor, 0)     \
		NAME(visibility, 0)     \
		NAME(stack_protect, 0) /* gcc */ \
		NAME(no_stack_protector, 0) /* clang */ \
		ALIAS("designated_init", desig_init, 0)     \
		ALIAS("__ucc_debug", ucc_debug, 0) /* logs out a message when handled */ \
		ALIAS("__cdecl", call_conv, 0)        \
		EXTRA_ALIAS("cdecl", call_conv)    \
		EXTRA_ALIAS("stdcall", call_conv)  \
		EXTRA_ALIAS("fastcall", call_conv) \
		EXTRA_ALIAS("warn_unused_result", warn_unused)

#endif
