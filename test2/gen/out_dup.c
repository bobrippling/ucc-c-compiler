// shouldn't clobber the pointer with the sub operation
f(int *p)
{
	--*p;
}
