#ifndef DYN_H
#define DYN_H

/* if your compiler doesn't support __builtin_types_compatible_p or __typeof,
 * define NO_DYN_CHECKS here
 */

#define UCC_STATIC_ASSERT(expr) (void)((int (*)[(expr) ? 1 : -1])0)

#ifndef NO_DYN_CHECKS
#  define UCC_TYPEOF(e) __typeof(e)

#  define UCC_TYPECHECK_BUILTIN(t, arg) \
	UCC_STATIC_ASSERT(__builtin_types_compatible_p(t, __typeof((void)0, arg)))

#  define UCC_TYPECHECK_VERBOSE(t, arg) \
	UCC_STATIC_ASSERT(_Generic((t)0, __typeof(arg): 1) == 1)

#define UCC_TYPECHECK UCC_TYPECHECK_BUILTIN

#else
#  define UCC_TYPEOF(e) (void)0
#  define UCC_TYPECHECK(t, arg) (void)0
#endif

#endif
