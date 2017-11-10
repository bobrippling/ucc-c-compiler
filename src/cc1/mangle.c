#include <stdio.h>

#include "../util/dynarray.h"
#include "../util/platform.h"
#include "../util/alloc.h"

#include "type.h"
#include "type_is.h"
#include "cc1.h"
#include "funcargs.h"

#include "mangle.h"

char *func_mangle(const char *name, type *fnty)
{
	char *pre, suff[8];

	pre = (cc1_backend != BACKEND_IR && fopt_mode & FOPT_LEADING_UNDERSCORE) ? "_" : "";
	*suff = '\0';

	if(fnty){
		funcargs *fa = type_funcargs(fnty);

		switch(fa->conv){
			case conv_fastcall:
				pre = "@";

			case conv_stdcall:
				snprintf(suff, sizeof suff,
						"@%d",
						dynarray_count(fa->arglist) * platform_word_size());

			case conv_x64_sysv:
			case conv_x64_ms:
			case conv_cdecl:
				break;
		}
	}

	if(*pre || *suff){
		return ustrprintf("%s%s%s", pre, name, suff);
	}

	return (char *)name;
}
