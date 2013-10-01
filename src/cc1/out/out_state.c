#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../../util/where.h"
#include "../../util/alloc.h"

#include "../data_structs.h"

#include "basic_block/bb.h"
#include "vstack.h"
#include "asm.h"
#include "impl.h" /* reg counts, etc */

#include "out_state.h"

struct out *out_state_new()
{
	struct out *o = umalloc(sizeof *o);
	return o;
}
