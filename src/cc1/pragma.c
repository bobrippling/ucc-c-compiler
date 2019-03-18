#include <stdlib.h>
#include <string.h>

#include "../util/where.h"

#include "pragma.h"

#include "../util/str.h"
#include "../util/alloc.h"
#include "warn.h"

#include <stdio.h>

char *ucc_namespace;

static void handle_pragma_stdc(const char *pragma, where *loc)
{
	/*
	 * #pragma STDC FP_CONTRACT <arg>
	 * #pragma STDC FENV_ACCESS <arg>
	 * #pragma STDC CX_LIMITED_RANGE <arg>
	 */
	cc1_warn_at(loc, unknown_pragma, "unhandled STDC pragma '%s'", pragma);
}

static void handle_pragma_ucc(const char *pragma, where *loc)
{
	if(!strncmp(pragma, "namespace ", 10)){
		free(ucc_namespace);
		ucc_namespace = ustrdup(pragma + 10);
		fprintf(stderr, "namespace \"%s\"\n", ucc_namespace);
	}else{
		cc1_warn_at(loc, unknown_pragma, "unknown ucc pragma '%s'", pragma);
	}
}

void pragma_handle(const char *pragma, where *loc)
{
	if(!strncmp(pragma, "STDC", 4)){
		handle_pragma_stdc(str_spc_skip(pragma + 4), loc);

	}else if(!strncmp(pragma, "ucc", 3)){
		handle_pragma_ucc(str_spc_skip(pragma + 3), loc);

	}else{
		/*
		 * #pragma GCC visibility <arg>
		 * #pragma align <arg>
		 * #pragma pack <arg>
		 * #pragma section <arg>
		 * #pragma unused <arg>
		 * #pragma weak <arg>
		 * #pragma attribute <arg>
		 */

		cc1_warn_at(loc, unknown_pragma, "unknown pragma '%s'", pragma);
	}
}
