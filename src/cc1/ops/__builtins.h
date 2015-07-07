#ifndef BUILTINS_H
#define BUILTINS_H

#define BUILTINS                                \
	BUILTIN("unreachable", unreachable)           \
	BUILTIN("trap", unreachable)                  \
	BUILTIN("types_compatible_p", compatible_p)   \
	BUILTIN("constant_p", constant_p)             \
	BUILTIN("frame_address", frame_address)       \
	BUILTIN("expect", expect)                     \
	BUILTIN("is_signed", is_signed)               \
	BUILTIN("nan",  nan)                          \
	BUILTIN("nanf", nan)                          \
	BUILTIN("nanl", nan)                          \
	BUILTIN("choose_expr", choose_expr)           \
	BUILTIN("va_start", va_start)                 \
	BUILTIN("va_arg", va_arg)                     \
	BUILTIN("va_end", va_end)                     \
	BUILTIN("va_copy", va_copy)

#endif
