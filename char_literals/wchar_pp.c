/* GCC may also define __WCHAR_UNSIGNED__.
 * Use L'\0' to give the expression the correct (unsigned) type.
 */
#ifdef __WCHAR_UNSIGNED__
#    define __WCHAR_MIN       L'\0'

/* Failing that, rely on the preprocessor's knowledge of the signedness of wchar_t */
#elif L'\0' - 1 > 0
#    define __WCHAR_MIN       L'\0'
#else
#    define __WCHAR_MIN       (-__WCHAR_MAX - 1)
#endif
