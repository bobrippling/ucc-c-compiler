#define _XOPEN_SOURCE 500 /* mkstemp */
#include <stdlib.h>
#include <unistd.h>

#include "tmpfile.h"
#include "alloc.h"
#include "path.h"

int tmpfile_prefix_out(const char *prefix, char **const fname)
{
	char *tmppath;
	int fd;
	const char *tmpdir = ucc_tmpdir();

	tmppath = ustrprintf("%s/%sXXXXXX", tmpdir, prefix);
	fd = mkstemp(tmppath);

	if(fd < 0){
		free(tmppath);
		tmppath = NULL;
	}

	if(fname)
		*fname = tmppath;
	else
		free(tmppath);

	return fd;
}
