/* // RUN: %ucc -fsyntax-only %s -std=c89
 */

f(char *restrict, int inline)
{
	return restrict[inline];
}
