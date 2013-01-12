/* FIXME: x-macros instead */
#define CASE_STR(s) case s: return #s
#define CASE_STR_PREFIX(pre, word) case pre ## _ ## word: return #word
