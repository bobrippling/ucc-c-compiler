int   *pi[3];
void  *pv[3];
int  (*fi[3])();
void (*fv[3])();

_Static_assert(sizeof(pi) == sizeof(int  *) * 3, "failed assert");
_Static_assert(sizeof(pv) == sizeof(void *) * 3, "failed assert");
_Static_assert(sizeof(fi) == sizeof(int  *) * 3, "failed assert");
_Static_assert(sizeof(fv) == sizeof(void *) * 3, "failed assert");

main()
{
}
