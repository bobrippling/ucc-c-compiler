// RUN: echo 'struct { int x:4, y:4; } bas = { 1, 2 };' | %ucc -o %t %s - -fpic
// RUN: %t

extern struct {
	int x : 4, y : 4;
} bas;

int main()
{
	/* this requires a GOT load, a normal load and then bitfield logic, which
	 * shouldn't be done twice despite the double indirection */
	if(bas.x != 1)
		return 1;

	/* ... same: */
	if(bas.y != 2)
		return 1;
	if(*(char *)&bas != 33)
		return 1;
	return 0;
}
