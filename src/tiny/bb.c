#include <stdlib.h>

#include "where.h"

#include "data_structs.h"
#include "type_ref.h"
#include "funcargs.h"

#include "basic_block.h"
#include "out.h"

int main()
{
	type_ref *ty_int = type_ref_cached_INT();

	basic_blk *bb_fn = out_func_prologue(
		type_ref_new_func(ty_int, funcargs_new()),
		/*stack=*/0,
		/*nargs=*/0,
		/*variadic=*/0,
		/*arg_offsets[]=*/0);

	out_push_l(bb_fn, ty_int, 5);
	out_pop_func_ret(bb_fn, ty_int);
}
