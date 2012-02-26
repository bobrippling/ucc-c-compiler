typedef int blkcnt_t;
typedef int blksize_t;
typedef int dev_t;
typedef int gid_t;
typedef int ino_t;
typedef int mode_t;
typedef int nlink_t;
typedef int off_t;
typedef int time_t;
typedef int uid_t;

#define long

struct stat
{
    dev_t     st_dev;     /* ID of device containing file */
    ino_t     st_ino;     /* inode number */

    mode_t    st_mode;    /* protection */
    nlink_t   st_nlink;   /* number of hard links */

    uid_t     st_uid;     /* user ID of owner */
    gid_t     st_gid;     /* group ID of owner */

		int       __pad0;

    dev_t     st_rdev;    /* device ID (if special file) */

    off_t     st_size;    /* total size, in bytes */

    blksize_t st_blksize; /* blocksize for file system I/O */
    blkcnt_t  st_blocks;  /* number of 512B blocks allocated */

#ifdef time_t_is_ll
    time_t    st_atime;   /* time of last access */
    time_t    st_mtime;   /* time of last modification */
    time_t    st_ctime;   /* time of last status change */
#else
		struct
		{
			int a, b;
		} st_atime, st_mtime, st_ctime;
#endif

		int __pad1;
};

int stat(const char *path, struct stat *buf);
