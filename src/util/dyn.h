#ifndef DYN_H
#define DYN_H

/* if your compiler doesn't support __builtin_types_compatible_p or __typeof,
 * define NO_DYN_CHECKS here
 */

#define UCC_STATIC_ASSERT(expr) (void)((int (*)[(expr) ? 1 : -1])0)

#ifndef NO_DYN_CHECKS
#  define UCC_TYPEOF(e) __typeof(e)
#  define UCC_TYPECHECK(t, arg) \
	UCC_STATIC_ASSERT(__builtin_types_compatible_p(t, __typeof(arg)))

#else
#  define UCC_TYPEOF(e) (void)0
#  define UCC_TYPECHECK(t, arg) (void)0
#endif

#endif
