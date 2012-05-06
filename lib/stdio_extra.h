FILE   *fopen(const char *, const char *);
int     fclose(FILE *);

size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);

int   fseek(FILE *, long, int);
void  rewind(FILE *);

long  ftell(FILE *);
int   fflush(FILE *);

int   getchar(void);
int   getc( FILE *);
int   fgetc(FILE *);
char *gets( char *);
char *fgets(char *, int, FILE *);

int   ungetc(int, FILE *);

int   putchar(int);
int   putc( int, FILE *);
int   fputc(int, FILE *);

int   fputs(const char *, FILE *);

FILE *fdopen(int, const char *);
int   fileno(FILE *);
void  clearerr(FILE *);
void  perror(const char *);

int   snprintf(char *, size_t, const char *, ...);
int   vprintf(const char *, va_list);
int   vfprintf(FILE *, const char *, va_list);
int   vsnprintf(char *, size_t, const char *, va_list);

int   scanf(const char *, ...);
int   fscanf(FILE *, const char *, ...);
int   sscanf(const char *, const char *, int ...);
int   vscanf(const char *, va_list);
int   vfscanf(FILE *, const char *, va_list);
int   vsscanf(const char *, const char *, va_list arg);


int   rename(const char *, const char *);

int pclose(FILE *);
FILE *popen(const char *, const char *);

int fseeko(FILE *, off_t, int);
int fsetpos(FILE *, const fpos_t *);
off_t ftello(FILE *);
FILE *tmpfile(void);
char *tmpnam(char *);
char *tempnam(const char *, const char *);
int sprintf(char *, const char *, ...);
int vsprintf(char *, const char *, va_list);
void setbuf(FILE *, char *);
int setvbuf(FILE *, char *, int, size_t);

flock();
