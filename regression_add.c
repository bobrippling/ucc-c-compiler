extern char _errs[23];

const char *strerror(int eno)
{
	return _errs[eno];
}
