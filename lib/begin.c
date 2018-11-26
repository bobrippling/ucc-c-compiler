#include "unistd.h" /* environ, __progname */
#include "stdlib.h" /* exit() */

_Noreturn
void __libc_start_main(
		int (*main)(int, char **, char **),
		int argc, char **argv);

_Noreturn void __libc_start_main(
		int (*main)(int, char **, char **),
		int argc, char **argv)
{
	__progname = argv[0];
	environ = argv + argc + 1;
	exit(main(argc, argv, environ));
}
