#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#define ATTRIBUTES    \
		NAME(format)         \
		NAME(unused)         \
		NAME(warn_unused)    \
		NAME(section)        \
		NAME(enum_bitmask)   \
		NAME(noreturn)       \
		NAME(noderef)        \
		NAME(nonnull)        \
		NAME(packed)         \
		NAME(sentinel)       \
		NAME(aligned)        \
		NAME(weak)           \
		NAME(cleanup)        \
		ALIAS("designated_init", desig_init)     \
		ALIAS("__ucc_debug", ucc_debug) /* logs out a message when handled */ \
		ALIAS("__cdecl", call_conv)        \
		EXTRA_ALIAS("cdecl", call_conv)    \
		EXTRA_ALIAS("stdcall", call_conv)  \
		EXTRA_ALIAS("fastcall", call_conv) \
		EXTRA_ALIAS("warn_unused_result", warn_unused)

#endif
