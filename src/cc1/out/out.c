#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "../cc1.h"
#include "../../util/platform.h"
#include "../sue.h"

#define asm_operand_kind(o, t) ((o)->impl == asm_operand_ ##t)

static int asm_flushing = 0;
