#include "tree.h"
#include "gen_str.h"
#include "gen_asm.h"

void gen(function **funcs, void (*f)(function *))
{
	function **iter;

	for(iter = funcs; *iter; iter++)
		f(*iter);
}
