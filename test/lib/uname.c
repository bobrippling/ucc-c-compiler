#include <sys/utsname.h>

main()
{
	struct utsname a;

	if(uname(&a))
		return 5;

#define P(x) printf(#x " = \"%s\"\n", a.x)

	P(sysname);
	P(nodename);
	P(release);
	P(version);
	P(machine);
	P(domainname);
}
