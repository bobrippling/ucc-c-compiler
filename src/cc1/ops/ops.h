#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "../data_structs.h"

#include "expr_addr.h"
#include "expr_assign.h"
#include "expr_cast.h"
#include "expr_comma.h"
#include "expr_funcall.h"
#include "expr_identifier.h"
#include "expr_if.h"
#include "expr_op.h"
#include "expr_sizeof.h"
#include "expr_val.h"

#include "../cc1.h"
#include "../asm.h"
#include "../fold.h"
#include "../const.h"
#include "../gen_str.h"
#include "../gen_asm.h"
