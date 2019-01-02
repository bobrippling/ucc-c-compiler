#include <string.h>

#include "dynarray.h"
#include "path.h"

static void canon_add(char ***pents, char *ent)
{
	if(!strcmp(ent, "."))
		return; /* skip */

	if(!strcmp(ent, "..")){
		if(*pents){
			unsigned n = dynarray_count(*pents);
			if(!strcmp((*pents)[n - 1], "..")){
				/* ../.. - can't change, fall through to add */
			}else{
				(void)dynarray_pop(char *, pents);
				return;
			}
		}else{
			/* the first is ".." */
		}
	}

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
	const int begin_slash = *path == '/';

	for(p = path; *p; p++){
		if(*p == '/'){
			*p = '\0';
			if(p > last)
				canon_add(&ents, last);
			last = p + 1;
		}
	}
	if(p > last)
		canon_add(&ents, last);
	else
		trailing_slash = 1;

	dest = path;
	if(begin_slash)
		*dest++ = '/';
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

	dynarray_free(char **, ents, NULL);

	return path;
}
