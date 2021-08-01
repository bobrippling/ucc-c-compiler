// RUN: %check %s -Wgnu

main()
{
	char *p = __FUNCTION__; // CHECK: warning: use of GNU __FUNCTION__
i:
	&& i; // CHECK: warning: use of GNU address-of-label
}
