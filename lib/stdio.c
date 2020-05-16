#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"
#include "sys/fcntl.h"
#include "assert.h"
#include "ctype.h"
#include "limits.h"

#include "ucc_attr.h"

#define FILE_FUN(f) ((f)->f_read || (f)->f_write)

#define PRINTF_ENABLE_PADDING
//#define FLOAT_SUPPORT
//#define LONG_LONG_SUPPORT

typedef __cleaned_va_list cleaned_va_list;

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
	__stdio_read  *f_read;
	__stdio_write *f_write;
	__stdio_seek  *f_seek;
	__stdio_close *f_close;

	/*
	char buf_write[256], buf_read[256];
	char *buf_write_p, *buf_read_p;
	*/
};
#define FILE_INIT(_fd) { .status = file_status_fine, .fd = _fd }

static FILE _stdin  = FILE_INIT(0);
static FILE _stdout = FILE_INIT(1);
static FILE _stderr = FILE_INIT(2);

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* Private */
static void fprintn_r(FILE *f, uintmax_t n, int base, unsigned ty_sz)
{
	uintmax_t d = n / base;
	if(d)
		fprintn_r(f, d, base, ty_sz);
	fwrite("0123456789abcdef" + n % base, sizeof(char), 1, f);
}

static void fprintn(FILE *f, uintmax_t n, int base, int is_signed, unsigned ty_sz)
{
	if(is_signed){
		// if we have an int/short/char, sign extend
		if(ty_sz < sizeof(uintmax_t)){
			// force sign extension - signed right shift
			n <<= (sizeof(long) - ty_sz) * CHAR_BIT;
			n = (intmax_t)n >> ((sizeof(long) - ty_sz) * CHAR_BIT);
		}

		if((intmax_t)n < 0){
			fwrite("-", sizeof(char), 1, f);
			n = -(intmax_t)n;
		}
	}

	fprintn_r(f, n, base, ty_sz);
}

static void fprintd(FILE *f, uintmax_t n, int is_signed, unsigned ty_sz)
{
	fprintn(f, n, 10, is_signed, ty_sz);
}

static void fprintx(FILE *f, uintmax_t n, int is_signed, unsigned ty_sz)
{
	fprintn(f, n, 16, is_signed, ty_sz);
}

static void fprinto(FILE *f, uintmax_t n, int is_signed, unsigned ty_sz)
{
	fprintn(f, n, 8, is_signed, ty_sz);
}

#ifdef FLOAT_SUPPORT
static void fprintfp(FILE *f, double d)
{
	const int mantissa = d;
	const int decimal_100 = (d - mantissa) * 100;

	fprintf(f, "%d.%02d", mantissa, decimal_100);
}
#endif

/* Public */
int feof(FILE *f)
{
	return f->status == file_status_eof;
}

int ferror(FILE *f)
{
	return f->status == file_status_err;
}

static void fopen_init(FILE *f, int fd)
{
	f->fd = fd;
	f->status = file_status_fine;
}

static int fopen2(FILE *f, const char *path, const char *smode)
{
	int fd, mode;
	int got_primary;

#define PRIMARY_CHECK() \
	if(got_primary) \
		goto inval; \
	got_primary = 1

	got_primary = mode = 0;

	if(!*smode)
		goto inval;

	while(*smode)
		switch(*smode++){
			case 'b':
				break;
			case 'r':
				PRIMARY_CHECK();
				mode |= O_RDONLY;
				break;
			case 'w':
				PRIMARY_CHECK();
				mode |= O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case 'a':
				PRIMARY_CHECK();
				mode |= O_WRONLY | O_CREAT; /* ? */
				break;
			case '+':
				mode &= ~(O_WRONLY | O_RDONLY);
				mode |= O_RDWR;
				break;
			case 'x':
				/* C11 exclusive create + open-exclusive */
				PRIMARY_CHECK();
				mode |= O_CREAT | O_EXCL;
				break;
			default:
inval:
				errno = EINVAL;
				return 1;
		}

#undef PRIMARY_CHECK

	fd = open(path, mode, 0644);
	if(fd == -1)
		return 1;

	fopen_init(f, fd);

	return 0;
}

static int fclose2(FILE *f)
{
	if(fflush(f))
		return EOF;

	if(FILE_FUN(f)){
		__stdio_close *c = f->f_close;
		return c ? c(f->cookie) : 0;
	}

	return close(fileno(f)) == 0 ? 0 : EOF;
}

FILE *funopen(void *cookie, __stdio_read *r, __stdio_write *w, __stdio_seek *s, __stdio_close *c)
{
	FILE *f;

	if(!r && !w){
		errno = EINVAL;
		return NULL;
	}

	f = malloc(sizeof *f);
	if(!f)
		return NULL;

	f->fd = -1;
	f->status = file_status_fine;

	f->cookie = cookie;
	f->f_read = r;
	f->f_write = w;
	f->f_seek = s;
	f->f_close = c;

	return f;
}

FILE *fropen(void *cookie, __stdio_read *rfn)
{
	return funopen(cookie, rfn, NULL, NULL, NULL);
}

FILE *fwopen(void *cookie, __stdio_write *wfn)
{
	return funopen(cookie, NULL, wfn, NULL, NULL);
}

FILE *fopen(const char *path, const char *mode)
{
	FILE *f = malloc(sizeof *f);
	if(!f)
		return NULL;

	if(fopen2(f, path, mode)){
		free(f);
		return NULL;
	}
	return f;
}

FILE *freopen(const char *path, const char *mode, FILE *f)
{
	if(fclose2(f))
		goto err;

	if(fopen2(f, path, mode))
		goto err;

	return f;
err:
	free(f);
	return NULL;
}

FILE *fdopen(int fd, const char *mode __unused)
{
	FILE *f = malloc(sizeof *f);
	if(!f)
		return NULL;
	fopen_init(f, fd);
	return f;
}

int fclose(FILE *f)
{
	int r = fclose2(f);
	free(f);
	return r;
}

int fseek(FILE *stream, long offset, int whence)
{
	if(FILE_FUN(stream)){
		__stdio_seek *s = stream->f_seek;
		if(s)
			return s(stream->cookie, offset, whence);
		errno = EINVAL;
		return -1;
	}

	return (int)lseek(fileno(stream), offset, whence);
}

long ftell(FILE *stream)
{
	return fseek(stream, 0, SEEK_CUR);
}

void rewind(FILE *stream)
{
	fseek(stream, 0, SEEK_SET);
}

int fgetpos(FILE *stream, fpos_t *pos)
{
	fpos_t p = (fpos_t)ftell(stream);

	if(p == (fpos_t)-1)
		return -1;

	*pos = p;
	return 0;
}

int fsetpos(FILE *stream, fpos_t *pos)
{
	return fseek(stream, *pos, SEEK_SET);
}

int fflush(FILE *f __unused)
{
	return 0;
}

int fputc(int c, FILE *f)
{
	return fwrite(&c, sizeof(char), 1, f) == 1 ? c : EOF;
}

int putchar(int c)
{
	return fputc(c, stdout);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	ssize_t n;

	if(FILE_FUN(stream)){
		if(stream->f_read)
			return stream->f_read(stream->cookie, ptr, size * nmemb);

		errno = EINVAL;
		return 0;
	}

	n = read(fileno(stream), ptr, size * nmemb);
	if(n == 0){
		stream->status = file_status_eof;
	}else if(n < 0){
		stream->status = file_status_err;
	}else{
		return (size_t)n;
	}
	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if(FILE_FUN(stream)){
		if(stream->f_write)
			return stream->f_write(stream->cookie, ptr, size * nmemb);

		errno = EINVAL;
		return 0;
	}

	ssize_t n = write(fileno(stream), ptr, size * nmemb);
	return n > 0 ? (size_t)n : 0;
}

int vfprintf(FILE *file, const char *fmt, va_list ap)
{
	const char *buf  = fmt;
	int buflen = 0;

	if(!fmt)
		return 0;

	while(*fmt){
		if(*fmt == '%'){
#ifdef PRINTF_ENABLE_PADDING
			int pad = 0;
#endif

			if(buflen)
				fwrite(buf, sizeof *buf, buflen, file); // TODO: errors

			fmt++;

#ifdef PRINTF_ENABLE_PADDING
			if(*fmt == '0'){
				// %0([0-9]+)d
				fmt++;
				pad = 0;
				while(isdigit(*fmt)){
					pad += *fmt - '0';
					fmt++;
				}
			}
#endif

			int lcount = 0;
			int is_z = 0;
			while(*fmt == 'l')
				lcount++, fmt++;
			if(*fmt == 'z')
				is_z = 1, fmt++;

			if(lcount > 2)
				goto wat;

			switch(*fmt){
				case 's':
				{
					const char *s = va_arg(ap, const char *);
					if(!s)
						s = "(null)";
					fwrite(s, sizeof *s, strlen(s), file);
					break;
				}
				case 'c':
					fputc(va_arg(ap, int), file);
					break;
				case 'u':
				case 'd':
				case 'x':
				case 'o':
				{
					static void (*printers[])(FILE *, uintmax_t, int, unsigned) = {
						['u'] = fprintd,
						['d'] = fprintd,
						['x'] = fprintx,
						['o'] = fprinto,
					};

					uintmax_t n;
					if(is_z || lcount == 1){
						n = va_arg(ap, long);
					}else if(lcount == 0){
						n = va_arg(ap, int);
					}else{
						n =
#ifdef LONG_LONG_SUPPORT
							va_arg(ap, long long)
#else
							va_arg(ap, long)
#endif
							;
					}

#ifdef PRINTF_ENABLE_PADDING
					if(pad){
						if(n){
							int len = 0;
							uintmax_t copy = n;

							while(copy){
								copy /= 10;
								len++;
							}

							while(pad-- > len)
								putchar('0');
						}else{
							while((--pad))
								putchar('0');
						}
					}
#endif

					printers[*fmt](file, n, *fmt == 'd',
							lcount ? sizeof(long) : sizeof(int));
					break;
				}
				case 'p':
				{
					void *p = va_arg(ap, void *);
					if(p){
						/* TODO - intptr_t */
						fputs("0x", file);
						fprintx(file, (long)p, 0, sizeof(long));
					}else{
						fputs("(nil)", file);
					}
					break;
				}
				case 'f':
#ifdef FLOAT_SUPPORT
					switch(lcount){
						case 0:
							/* hacky float support, mainly for debugging */
							fprintfp(file, va_arg(ap, double));
							break;
						case 1:
						{
							//long double d = va_arg(ap, long double);
							assert(0 && "TODO: long double printf");
							break;
						}
						case 2: /* 2 is max for lcount */
							goto wat;
					}
#else
					(void)va_arg(ap, long);
#endif
					break;

				default:
wat:
					/* default to just printing the char */
					fwrite(fmt, sizeof *fmt, 1, file);
			}

			buf = fmt + 1;
			buflen = 0;
		}else{
			buflen++;
		}
		fmt++;
	}

	if(buflen)
		fwrite(buf, sizeof *buf, buflen, file);


	return 0;
}

int vprintf(const char *fmt, va_list l)
{
	return vfprintf(stdout, fmt, l);
}

int dprintf(int fd, const char *fmt, ...)
{
	cleaned_va_list l;

	va_start(l, fmt);

	return vfprintf(
			&(FILE){
				.fd = fd,
				.status = file_status_fine,
			},
			fmt,
			l);
}

int fprintf(FILE *file, const char *fmt, ...)
{
	cleaned_va_list l;

	va_start(l, fmt);
	return vfprintf(file, fmt, l);
}

int printf(const char *fmt, ...)
{
	cleaned_va_list l;

	va_start(l, fmt);
	return vfprintf(stdout, fmt, l);
}

int fputs(const char *s, FILE *f)
{
	const size_t len = strlen(s);
	return fwrite(s, sizeof *s, len, f) == len ? 1 : EOF;
}

int puts(const char *s)
{
	return printf("%s\n", s) > 0 ? 1 : EOF;
}

int getchar(void)
{
	return fgetc(stdin);
}

int fgetc(FILE *f)
{
	int ch;
	return fread(&ch, 1, 1, f) == 1 ? ch : EOF;
}

char *fgets(char *s, int l, FILE *f)
{
	size_t r;

	r = fread(s, l - 1, 1, f);

	if(r == 0)
		return NULL;

	/* FIXME: read only one line at a time */

	s[MIN((unsigned)l, r)] = '\0';

	return s;
}

/* error */
void perror(const char *s)
{
	fprintf(stderr, "%s%s%s\n",
			s ? s    : "",
			s ? ": " : "",
			strerror(errno));
}

/* file system */

int remove(const char *f)
{
	if(unlink(f)){
		if(errno == EISDIR)
			return rmdir(f);
		return -1;
	}
	return 0;
}

int fileno(FILE *f)
{
	return f->fd;
}
