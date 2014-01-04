#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>

#include "specs.h"
#include "ucc.h"
#include "../util/alloc.h"

#define BUF_LEN 4096

#define SPECS_FILE "ucc.specs"

struct specs specs;

static void find_us(char path[], ssize_t len)
{
	ssize_t n;

	memset(path, 0, len);

	n = readlink("/proc/self/exe", path, len);

	if(n == -1){
		/* not linux, try something else */
	}else if(n == len && path[len - 1]){
		die("readlink(self), path too large");
	}else{
		/* readlink was fine */
		return;
	}

	/* search $PATH for us */
	die("TODO");
}

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
	char path[4096];
	char buf[BUF_LEN];
	char *p;
	FILE *f;

	find_us(path, sizeof(path) - strlen(SPECS_FILE));

	/* specs file */
	p = strrchr(path, '/');
	if(p)
		*++p = '\0';

	strcat(path, SPECS_FILE);

	f = fopen(path, "r");
	if(!f)
		die("open \"%s\":", path);

	while(specs_line(f, path, buf));

	fclose(f);

	if(!specs.cc1 || !specs.cpp)
		die("missing line in %s", path);
}
