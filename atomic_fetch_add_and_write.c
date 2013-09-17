typedef struct {
	_Atomic int i;
} A;

f(A *p)
{
	p->i = p->i + 5;

	return p->i;
}
