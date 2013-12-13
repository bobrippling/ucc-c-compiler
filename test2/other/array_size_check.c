// RUN: %ucc -o %t %s
// RUN: %t

#define passert(a, b) if(a != b){printf("%d != %d (%s == %s)\n", a, b, #a, #b); abort();}
int printf(), abort();
main()
{
	int   *pi[3];
	void  *pv[3];
	int  (*fi[3])();
	void (*fv[3])();

	int (*p_to_ar)[3];

	passert(sizeof(pi), sizeof(int  *) * 3);
	passert(sizeof(pv), sizeof(void *) * 3);
	passert(sizeof(fi), sizeof(int  *) * 3);
	passert(sizeof(fv), sizeof(void *) * 3);

	passert(sizeof(p_to_ar), sizeof(void *));

	return 0;
}

int   *pi[3];
void  *pv[3];
int  (*fi[3])();
void (*fv[3])();

#define passert2(x) _Static_assert((x), #x " failed")

passert2(sizeof(pi) == sizeof(int  *) * 3);
passert2(sizeof(pv) == sizeof(void *) * 3);
passert2(sizeof(fi) == sizeof(int  *) * 3);
passert2(sizeof(fv) == sizeof(void *) * 3);
