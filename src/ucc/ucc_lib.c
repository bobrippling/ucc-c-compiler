#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_lib.h"

#include "../util/alloc.h"
#include "../util/dynarray.h"

#define LIB_PATH "../../lib/"

#define LIBS          \
		"stdio.o",        \
		"stdlib.o",       \
		"string.o",       \
		"unistd.o",       \
		"syscall.o",      \
		"signal.o",       \
		"assert.o",       \
		"ctype.o",        \
		"dirent.o",       \
		"alloca.o",       \
		"sys/fcntl.o",    \
		"sys/wait.o",     \
		"sys/mman.o",     \
		"sys/socket.o",   \
		"sys/utsname.o",  \
		"sys/select.o",   \
		"sys/time.o"

#define lib_actual_path(lib) actual_path(LIB_PATH, lib)

char **objfiles_stdlib(void)
{
	static char **ret = NULL;

	if(!ret){
		const char *names[] = {
			LIBS,
			NULL
		};

		int i;

		for(i = 0; names[i]; i++)
			dynarray_add(&ret, lib_actual_path(names[i]));
	}

	return ret;
}

char *objfiles_start(void)
{
	static char *p;
  if(!p)
		p = lib_actual_path("crt.o");
	return p;
}
