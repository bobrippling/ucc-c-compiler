// RUN: %ucc -E %s >/dev/null
// RUN: %ucc -C -E %s >/dev/null
// RUN: %ucc -CC -E %s >/dev/null

/*
#if bad
*/

/* #if bad */

#define good howdy /* yo
*/

#if bad /*
*/
#endif

/* #endif */
/*
#endif */

/*
#endif
*/

#if 1
#else
#error bad
#endif /*
*/

#ifndef good
#  error bad
#endif

#if 1
#endif /*
|| */



/* macro in comment: good */
