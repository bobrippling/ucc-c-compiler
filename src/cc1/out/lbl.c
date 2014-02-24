#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../../util/alloc.h"
#include "../../util/util.h"
#include "common.h"
#include "lbl.h"
#include "../../as_cfg.h"

static int label_last    = 1,
					 str_last      = 1,
					 switch_last   = 1,
					 flow_last     = 1,
					 block_last    = 1,
					 static_last   = 1;

char *out_label_block(const char *funcsp)
{
	int len;
	char *ret;

	len = strlen(funcsp) + 16;
	ret = umalloc(len);

	SNPRINTF(ret, len, "%s.block_%d", funcsp, block_last++);

	return ret;
}

char *out_label_code(const char *fmt)
{
	int len;
	char *ret;

	len = strlen(fmt) + 10;
	ret = umalloc(len + 1);

	SNPRINTF(ret, len, ASM_PLBL_PRE "%s.%d", fmt, label_last++);

	return ret;
}

char *out_label_data_store(enum out_label_store ty)
{
	char *ret = umalloc(16);
	const char *pre;
	switch(ty){
		case STORE_P_CHAR: pre = "str"; break;
		case STORE_P_WCHAR: pre = "wstr"; break;
		case STORE_COMP_LIT: pre = "data"; break;
		case STORE_FLOAT: pre = "float"; break;
	}
	SNPRINTF(ret, 16, "%s.%d", pre, str_last++);
	return ret;
}

char *out_label_static_local(const char *funcsp, const char *spel)
{
	char *ret;
	int len;

	UCC_ASSERT(funcsp, "no spel for %s", __func__);

	len = strlen(funcsp) + strlen(spel) + 16;
	ret = umalloc(len);
	SNPRINTF(ret, len, "%s.static%d_%s", funcsp, static_last++, spel);
	return ret;
}

char *out_label_goto(char *func, char *lbl)
{
	int len = strlen(func) + strlen(lbl) + 6;
	char *ret = umalloc(len);
	SNPRINTF(ret, len, ASM_PLBL_PRE "%s.%s", func, lbl);
	return ret;
}

char *out_label_case(enum out_label_type lbltype, int val)
{
	int len;
	char *ret = umalloc(len = 15 + 32);
	switch(lbltype){
		case CASE_DEF:
			SNPRINTF(ret, len, ASM_PLBL_PRE "case_%d_default", switch_last);
			break;

		case CASE_CASE:
		case CASE_RANGE:
		{
			const char *extra = "";
			if(val < 0){
				val = -val;
				extra = "m";
			}
			SNPRINTF(ret, len, ASM_PLBL_PRE "case%s_%d_%s%d", lbltype == CASE_RANGE ? "_rng" : "", switch_last, extra, val);
			break;
		}
	}
	switch_last++;
	return ret;

}

char *out_label_flow(const char *fmt)
{
	int len = 16 + strlen(fmt);
	char *ret = umalloc(len);
	SNPRINTF(ret, len, ASM_PLBL_PRE "flow_%s_%d", fmt, flow_last++);
	return ret;
}

char *out_dbg_func_end(const char *fn)
{
	return ustrprintf(ASM_PLBL_PRE "funcend_%s", fn);
}
