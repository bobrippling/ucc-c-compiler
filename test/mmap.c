#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>

#define SZ 4096

main()
{
	void *p = mmap(NULL, SZ,
			PROT_READ   | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			-1, 0);

	printf("mmap() = %p\n", p);
	printf("munmap() = %d\n", munmap(p, SZ));
}
