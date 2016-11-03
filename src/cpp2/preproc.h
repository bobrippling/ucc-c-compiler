#ifndef PREPROC_H
#define PREPROC_H

struct macro
{
	char *spel;
	char *replace;
};

struct file_stack
{
	FILE *file;
	char *fname;
	int line_no;
};

extern struct file_stack file_stack[];
extern int file_stack_idx;

void preprocess(void);
void preproc_push(FILE *f, const char *fname);
int preproc_in_include(void);

void preproc_emit_line_info(int lineno, const char *fname);

#endif
