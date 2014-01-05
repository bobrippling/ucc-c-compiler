#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>

#include "specs.h"
#include "ucc.h"
#include "ucc_path.h"
#include "../util/alloc.h"

#define BUF_LEN 4096

#define SPECS_FILE "ucc.specs"

struct specs specs;

static int specs_line(FILE *f, char *path, char buf[BUF_LEN])
{
	char *p;

	if(!fgets(buf, BUF_LEN, f)){
		if(feof(f))
			return 0;

		die("read(%s):", path);
	}

	p = strchr(buf, '\n');
	if(p)
		*p = '\0';

	p = strchr(buf, ':');
	if(!p)
		die("no colon in specs line (%s)", buf);

	*p++ = '\0';
	while(*p == ' ')
		p++;

	if(!strcmp(buf, "cc1"))
		specs.cc1 = ustrdup(p);
	else if(!strcmp(buf, "cpp"))
		specs.cpp = ustrdup(p);
	else
		die("unknown specs line \"%s\"", buf);

	return 1;
}

void specs_read(void)
{
	char *path = ustrprintf("%s/" SPECS_FILE, ucc_argv0_path());
	char buf[BUF_LEN];
	FILE *f;

	f = fopen(path, "r");
	if(!f)
		die("open \"%s\":", path);

	while(specs_line(f, path, buf));

	fclose(f);

	if(!specs.cc1 || !specs.cpp)
		die("missing line in %s", path);
}
