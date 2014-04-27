#include <string.h>

#include "dynarray.h"
#include "path.h"

static void canon_add(char ***pents, char *ent, int *last_was_dotdot)
{
	if(!strcmp(ent, "..")){
		if(*last_was_dotdot){
			/* ../.. - can't change, fall through to add */
		}else if(*pents){
			dynarray_pop(char *, pents);
			return;
		}else{
			/* the first is ".." */
		}
		*last_was_dotdot = 1;
	}else{
		*last_was_dotdot = 0;
	}

	if(!strcmp(ent, "."))
		; /* skip */
	else
		dynarray_add(pents, ent);
}

char *canonicalise_path(char *path)
{
	/* replace ^.\b and /.\b with ""
	 * replace "x/..\b" with ""
	 */
	char **ents = NULL;
	char *p, *last = path, *dest;
	char **i;
	int trailing_slash = 0;
	int dotdot = 0;

	for(p = path; *p; p++){
		if(*p == '/'){
			*p = '\0';
			if(p > last)
				canon_add(&ents, last, &dotdot);
			last = p + 1;
		}
	}
	if(p > last)
		canon_add(&ents, last, &dotdot);
	else
		trailing_slash = 1;

	dest = path;
	for(i = ents; i && *i; i++){
		/* don't use sprintf,
		 * we want to avoid the \0
		 * overwriting the next entry */

		size_t len = strlen(*i);
		memmove(dest, *i, len);
		dest += len;
		*dest++ = '/';
	}

	dest[trailing_slash ? 0 : -1] = '\0';

	dynarray_free(char **, &ents, NULL);

	return path;
}
