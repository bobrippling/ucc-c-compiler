#include <stdio.h>
#include <sys/stat.h>

#define SIZE(x) printf("sizeof(" #x ") = %d\n", sizeof(x))
#define INFO(f) SIZE(st.f)

main()
{
	struct stat st;
	INFO(st_dev);
	INFO(st_ino);
	INFO(st_mode);
	INFO(st_nlink);
	INFO(st_uid);
	INFO(st_gid);
	INFO(st_rdev);
	INFO(st_size);
	INFO(st_blksize);
	INFO(st_blocks);
	INFO(st_atime);
	INFO(st_mtime);
	INFO(st_ctime);
	SIZE(struct stat);
	return 0;
}
