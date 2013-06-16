// RUN: %ucc -E %s | grep -F '2 + 3 * 5 * 5'

#define G(g) g * 5
#define F(x, y) x + G(y) //x + #y + y ## x
F(2, G(3))
/* this checks that G can be double-y expanded,
 * and isn't blued by the initial argument substitution
 */
