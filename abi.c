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
