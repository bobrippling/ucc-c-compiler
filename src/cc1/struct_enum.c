#include <stdio.h>
#include <stdlib.h>

#include "../util/alloc.h"
#include "struct_enum.h"

void st_en_set_spel(char **dest, char *spel, const char *desc)
{
	if(spel){
		*dest = spel;
	}else{
		*dest = umalloc(32);
		snprintf(*dest, 32, "<anon %s %p>", desc, (void *)dest);
	}
}
