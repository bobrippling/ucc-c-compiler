_Atomic struct {
	char buf[4096];
} a, a2;

_Atomic(struct {
	char buf[4096];
}) b;

f()
{
	//a.buf[3] = 3;
	a = a2;
}
