#include <stdlib.h>
#include <unistd.h>

#include "colour.h"

#include "tristate.h"

static enum tristate from_option = TRI_UNSET;

int colour_enabled(int fd)
{
	switch(from_option){
		case TRI_UNSET:
			break;
		case TRI_FALSE:
			return 0;
		case TRI_TRUE:
			return 1;
	}

	const char *no_color = getenv("NO_COLOR");
	if(no_color)
		return 0;

	return isatty(fd);
}

void colour_enable(int enable)
{
	from_option = enable ? TRI_TRUE : TRI_FALSE;
}
