#include "val.h"

out_val *outval_new(out_ctx *octx, out_val *from);
{
	if(from->retains == 0){
		/* reuse */
		return from;
	}else{
		out_val *ov = umalloc(sizeof *ov);
		memcpy_safe(ov, from);
		return ov;
	}
}
