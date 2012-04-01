typedef struct {
	int i, j;
	char s[5];
} a_t;

main()
{
	a_t q;
	a_t *p;

	p = &q;

	q.i = 5;
	p->i = -5;

	return (&q)->i + p->j;
}
