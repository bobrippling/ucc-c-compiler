#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#include "../type.h"
#include "../type_is.h"

#include "out.h"
#include "val.h"
#include "asm.h"
#include "impl.h"

out_val *out_op(out_ctx *octx, enum op_type binop, out_val *lhs, out_val *rhs)
{
	return impl_op(octx, binop, lhs, rhs);
}

out_val *out_op_unary(out_ctx *octx, enum op_type uop, out_val *exp)
{
	return impl_op_unary(octx, uop, exp);
}
