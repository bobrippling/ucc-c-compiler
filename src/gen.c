#include "tree.h"
#include "gen_str.h"
#include "gen_asm.h"

void gen(function **funcs)
{
	function **iter;

	for(iter = funcs; *iter; iter++)
		walk_fn(*iter);
}
