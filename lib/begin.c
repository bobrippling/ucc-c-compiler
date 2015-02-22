#include "unistd.h" /* environ */
#include "stdlib.h" /* exit() */

char **environ;
char *__progname;

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
