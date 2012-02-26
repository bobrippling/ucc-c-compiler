#ifdef __UCC__
# include "stat.h"
# include <assert.h>
#else
# include <sys/stat.h>
#endif

int main()
{
	struct stat st;
	printf("%d\n", sizeof(st));
	assert(sizeof(st) >= 140);
	stat("a.out", &st);

	printf("uid %d, gid %d\n", st.st_uid, st.st_gid);
}
