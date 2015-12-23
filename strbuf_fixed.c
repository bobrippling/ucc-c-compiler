#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "strbuf_fixed.h"

bool strbuf_fixed_vprintf(strbuf_fixed *buf, const char *fmt, va_list l)
{
	size_t space_req;
	size_t space_avail = buf->max - buf->current;

	if(buf->current == buf->max)
		return false;
	assert(buf->current < buf->max);

	space_req = 1 + vsnprintf(buf->str + buf->current, space_avail, fmt, l);
	/* +1 for '\0' */

	if(space_req <= space_avail){
		buf->current += space_req - 1;
		return true;
	}else{
		buf->current = buf->max;
		return false;
	}
}

bool strbuf_fixed_printf(strbuf_fixed *buf, const char *fmt, ...)
{
	va_list l;
	bool ret;
	va_start(l, fmt);
	ret = strbuf_fixed_vprintf(buf, fmt, l);
	va_end(l);
	return ret;
}

char *strbuf_fixed_detach(strbuf_fixed *buf)
{
	char *ret = buf->str;
	buf->current = 0;
	return ret;
}
