// RUN: %layout_check %s

typedef int (*fp)();
typedef struct __FILE FILE;

struct __FILE
{
	int fd;
	enum
	{
		file_status_fine,
		file_status_eof,
		file_status_err
	} status;

	void *cookie;
	fp *f_read;
	fp *f_write;
	fp *f_seek;
	fp *f_close;

	/*
	char buf_write[256], buf_read[256];
	char *buf_write_p, *buf_read_p;
	*/
};

FILE static stdin = {
	.status = file_status_fine,
	.fd = 1
};

int i = 2;

short j[] = { 2, 3 };
