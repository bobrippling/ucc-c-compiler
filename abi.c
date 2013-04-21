#ifndef __UCC__
struct __va_list_struct
{
	unsigned int gp_offset; // reg_count * 8
	unsigned int fp_offset; // sse_count * 16 + 48
	union
	{
		unsigned int overflow_offset; // stack size (bytes)
		char *overflow_arg_area; // converted from above to this in __va_start
	};
	char *reg_save_area; // uninit
};
#endif

enum __va_arg_type { __va_gen_reg, __va_float_reg, __va_stack };

void *__va_arg(struct __va_list_struct *ap,
               enum __va_arg_type arg_type,
               int size)
{
    size = (size + 7) & ~7;
    switch (arg_type) {
    case __va_gen_reg:
        if (ap->gp_offset < 48) {
            ap->gp_offset += 8;
            return ap->reg_save_area + ap->gp_offset - 8;
        }
        size = 8;
        goto use_overflow_area;

    case __va_float_reg:
        if (ap->fp_offset < 128 + 48) {
            ap->fp_offset += 16;
            return ap->reg_save_area + ap->fp_offset - 16;
        }
        size = 8;
        goto use_overflow_area;

    case __va_stack:
    use_overflow_area:
        ap->overflow_arg_area += size;
        return ap->overflow_arg_area - size;

    default:
        abort();
    }
}

pd(int i){printf("%d\n", i);}
