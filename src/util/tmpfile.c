#define _XOPEN_SOURCE 500 /* mkstemp */
#include <stdlib.h>
#include <unistd.h>

#include "tmpfile.h"
#include "alloc.h"

int tmpfile_prefix_out(const char *prefix, char **const fname)
{
	char *tmppath;
	int fd;
	char *tmpdir = getenv("TMPDIR");

#ifdef P_tmpdir
	if(!tmpdir)
		tmpdir = P_tmpdir;
#endif
	if(!tmpdir)
		tmpdir = "/tmp";

	tmppath = ustrprintf("%s/%sXXXXXX", tmpdir, prefix);
	fd = mkstemp(tmppath);

	if(fname)
		*fname = tmppath;
	else
		free(tmppath);

	return fd;
}
