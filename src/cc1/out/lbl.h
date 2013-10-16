#ifndef OUT_LBL_H
#define OUT_LBL_H

enum out_label_type
{
	CASE_CASE,
	CASE_DEF,
	CASE_RANGE
};

char *out_label_code(const char *fmt);
char *out_label_data_store(int str);
char *out_label_static_local(const char *funcsp, const char *spel);
char *out_label_goto(char *func, char *lbl);
char *out_label_case(enum out_label_type, int val);
char *out_label_flow(const char *fmt);
char *out_label_block(const char *funcsp);

char *out_label_dbg_type(void);

#endif
