#include "stdarg.h"

void __clean_va_list(va_list *p)
{
	va_end(*p);
}
