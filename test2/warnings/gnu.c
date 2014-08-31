// RUN: %check %s -Wgnu

main()
{
i:
	&& i; // CHECK: warning: use of GNU address-of-label
}
