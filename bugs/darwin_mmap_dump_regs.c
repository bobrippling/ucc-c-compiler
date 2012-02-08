#include <stdlib.h>
#include <sys/mman.h>
#include <ucc.h>

main()
{
	mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	__dump_regs();
}
