#ifndef DYN_H
#define DYN_H

/* if your compiler doesn't support __builtin_types_compatible_p or __typeof,
 * define NO_DYN_CHECKS here
 */

#ifndef NO_DYN_CHECKS
#  define UCC_TYPECHECK(t, arg) \
		(void)((int (*)[__builtin_types_compatible_p(t, __typeof(arg)) ? 1 : -1])NULL)
#else
#  define UCC_TYPECHECK(t, arg) (void)0
#endif

#endif
