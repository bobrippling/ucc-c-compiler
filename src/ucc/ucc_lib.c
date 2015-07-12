#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include "ucc.h"
#include "ucc_ext.h"
#include "ucc_lib.h"
#include "str.h"

#include "../config_driver.h"

#include "../util/alloc.h"
#include "../util/dynarray.h"

char **ld_stdlib_args(void)
{
	static char **ret;

	if(!ret)
		ret = strsplit(UCC_STDLIB, ":");

	return ret;
}

char **ld_crt_args(void)
{
	static char **ret;
  if(!ret)
		ret = strsplit(UCC_CRT, ":");
	return ret;
}
