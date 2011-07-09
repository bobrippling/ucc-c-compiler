#include "tree.h"
#include "gen_str.h"

void gen(function **funcs)
{
	function **iter;

	for(iter = funcs; *iter; iter++)
		print_fn(*iter);
}
