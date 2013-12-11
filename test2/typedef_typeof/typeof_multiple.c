// RUN: %layout_check %s

#define pointer(T)  typeof(T *)
#define array(T, N) typeof(T [N])

/* there was an ICE where we checked a type_ref's ref type before it was folded
 * (e.g. array of function check). this nesting checks that
 */

array (pointer (char), 4) y;
