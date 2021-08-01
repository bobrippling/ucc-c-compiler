// grob: this is from a pcc mailing list on Tue 3 Oct 2017

# 1 "aacquant.c"

# 1 "/usr/include//math.h" 3

# 1 "/usr/include//features.h" 3

# 8 "/usr/include//math.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 37 "/usr/include//bits/alltypes.h" 3
typedef float float_t;




typedef double double_t;
# 12 "/usr/include//math.h" 3

# 39 "/usr/include//math.h" 3
int __fpclassify(double);
int __fpclassifyf(float);
int __fpclassifyl(long double);

static inline unsigned __FLOAT_BITS(float __f)
{
union {float __f; unsigned __i;} __u;
__u.__f = __f;
return __u.__i;
}
static inline unsigned long long __DOUBLE_BITS(double __f)
{
union {double __f; unsigned long long __i;} __u;
__u.__f = __f;
return __u.__i;
}
# 81 "/usr/include//math.h" 3
int __signbit(double);
int __signbitf(float);
int __signbitl(long double);
# 96 "/usr/include//math.h" 3
static inline int __islessf(float_t __x, float_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x < __y; }
static inline int __isless(double_t __x, double_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x < __y; }
static inline int __islessl(long double __x, long double __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x < __y; }
static inline int __islessequalf(float_t __x, float_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x <= __y; }
static inline int __islessequal(double_t __x, double_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x <= __y; }
static inline int __islessequall(long double __x, long double __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x <= __y; }
static inline int __islessgreaterf(float_t __x, float_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x != __y; }
static inline int __islessgreater(double_t __x, double_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x != __y; }
static inline int __islessgreaterl(long double __x, long double __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x != __y; }
static inline int __isgreaterf(float_t __x, float_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x > __y; }
static inline int __isgreater(double_t __x, double_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x > __y; }
static inline int __isgreaterl(long double __x, long double __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x > __y; }
static inline int __isgreaterequalf(float_t __x, float_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x >= __y; }
static inline int __isgreaterequal(double_t __x, double_t __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x >= __y; }
static inline int __isgreaterequall(long double __x, long double __y) { return !(( sizeof((__x)) == sizeof(float) ? (__FLOAT_BITS((__x)) & 0x7fffffff) > 0x7f800000 : sizeof((__x)) == sizeof(double) ? (__DOUBLE_BITS((__x)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__x)) == 0) ? ((void)(__y),1) : ( sizeof((__y)) == sizeof(float) ? (__FLOAT_BITS((__y)) & 0x7fffffff) > 0x7f800000 : sizeof((__y)) == sizeof(double) ? (__DOUBLE_BITS((__y)) & -1ULL>>1) > 0x7ffULL<<52 : __fpclassifyl((__y)) == 0)) && __x >= __y; }
# 123 "/usr/include//math.h" 3
double      acos(double);
float       acosf(float);
long double acosl(long double);

double      acosh(double);
float       acoshf(float);
long double acoshl(long double);

double      asin(double);
float       asinf(float);
long double asinl(long double);

double      asinh(double);
float       asinhf(float);
long double asinhl(long double);

double      atan(double);
float       atanf(float);
long double atanl(long double);

double      atan2(double, double);
float       atan2f(float, float);
long double atan2l(long double, long double);

double      atanh(double);
float       atanhf(float);
long double atanhl(long double);

double      cbrt(double);
float       cbrtf(float);
long double cbrtl(long double);

double      ceil(double);
float       ceilf(float);
long double ceill(long double);

double      copysign(double, double);
float       copysignf(float, float);
long double copysignl(long double, long double);

double      cos(double);
float       cosf(float);
long double cosl(long double);

double      cosh(double);
float       coshf(float);
long double coshl(long double);

double      erf(double);
float       erff(float);
long double erfl(long double);

double      erfc(double);
float       erfcf(float);
long double erfcl(long double);

double      exp(double);
float       expf(float);
long double expl(long double);

double      exp2(double);
float       exp2f(float);
long double exp2l(long double);

double      expm1(double);
float       expm1f(float);
long double expm1l(long double);

double      fabs(double);
float       fabsf(float);
long double fabsl(long double);

double      fdim(double, double);
float       fdimf(float, float);
long double fdiml(long double, long double);

double      floor(double);
float       floorf(float);
long double floorl(long double);

double      fma(double, double, double);
float       fmaf(float, float, float);
long double fmal(long double, long double, long double);

double      fmax(double, double);
float       fmaxf(float, float);
long double fmaxl(long double, long double);

double      fmin(double, double);
float       fminf(float, float);
long double fminl(long double, long double);

double      fmod(double, double);
float       fmodf(float, float);
long double fmodl(long double, long double);

double      frexp(double, int *);
float       frexpf(float, int *);
long double frexpl(long double, int *);

double      hypot(double, double);
float       hypotf(float, float);
long double hypotl(long double, long double);

int         ilogb(double);
int         ilogbf(float);
int         ilogbl(long double);

double      ldexp(double, int);
float       ldexpf(float, int);
long double ldexpl(long double, int);

double      lgamma(double);
float       lgammaf(float);
long double lgammal(long double);

long long   llrint(double);
long long   llrintf(float);
long long   llrintl(long double);

long long   llround(double);
long long   llroundf(float);
long long   llroundl(long double);

double      log(double);
float       logf(float);
long double logl(long double);

double      log10(double);
float       log10f(float);
long double log10l(long double);

double      log1p(double);
float       log1pf(float);
long double log1pl(long double);

double      log2(double);
float       log2f(float);
long double log2l(long double);

double      logb(double);
float       logbf(float);
long double logbl(long double);

long        lrint(double);
long        lrintf(float);
long        lrintl(long double);

long        lround(double);
long        lroundf(float);
long        lroundl(long double);

double      modf(double, double *);
float       modff(float, float *);
long double modfl(long double, long double *);

double      nan(const char *);
float       nanf(const char *);
long double nanl(const char *);

double      nearbyint(double);
float       nearbyintf(float);
long double nearbyintl(long double);

double      nextafter(double, double);
float       nextafterf(float, float);
long double nextafterl(long double, long double);

double      nexttoward(double, long double);
float       nexttowardf(float, long double);
long double nexttowardl(long double, long double);

double      pow(double, double);
float       powf(float, float);
long double powl(long double, long double);

double      remainder(double, double);
float       remainderf(float, float);
long double remainderl(long double, long double);

double      remquo(double, double, int *);
float       remquof(float, float, int *);
long double remquol(long double, long double, int *);

double      rint(double);
float       rintf(float);
long double rintl(long double);

double      round(double);
float       roundf(float);
long double roundl(long double);

double      scalbln(double, long);
float       scalblnf(float, long);
long double scalblnl(long double, long);

double      scalbn(double, int);
float       scalbnf(float, int);
long double scalbnl(long double, int);

double      sin(double);
float       sinf(float);
long double sinl(long double);

double      sinh(double);
float       sinhf(float);
long double sinhl(long double);

double      sqrt(double);
float       sqrtf(float);
long double sqrtl(long double);

double      tan(double);
float       tanf(float);
long double tanl(long double);

double      tanh(double);
float       tanhf(float);
long double tanhl(long double);

double      tgamma(double);
float       tgammaf(float);
long double tgammal(long double);

double      trunc(double);
float       truncf(float);
long double truncl(long double);
# 372 "/usr/include//math.h" 3
extern int signgam;

double      j0(double);
double      j1(double);
double      jn(int, double);

double      y0(double);
double      y1(double);
double      yn(int, double);





double      drem(double, double);
float       dremf(float, float);

int         finite(double);
int         finitef(float);

double      scalb(double, double);
float       scalbf(float, float);

double      significand(double);
float       significandf(float);

double      lgamma_r(double, int*);
float       lgammaf_r(float, int*);

float       j0f(float);
float       j1f(float);
float       jnf(int, float);

float       y0f(float);
float       y1f(float);
float       ynf(int, float);
# 23 "aacquant.c"

# 1 "/usr/include//stdlib.h" 3

# 1 "/usr/include//features.h" 3

# 8 "/usr/include//stdlib.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 18 "/usr/include//bits/alltypes.h" 3
typedef int wchar_t;
# 101 "/usr/include//bits/alltypes.h" 3
typedef unsigned long size_t;
# 19 "/usr/include//stdlib.h" 3


int atoi (const char *);
long atol (const char *);
long long atoll (const char *);
double atof (const char *);

float strtof (const char *restrict, char **restrict);
double strtod (const char *restrict, char **restrict);
long double strtold (const char *restrict, char **restrict);

long strtol (const char *restrict, char **restrict, int);
unsigned long strtoul (const char *restrict, char **restrict, int);
long long strtoll (const char *restrict, char **restrict, int);
unsigned long long strtoull (const char *restrict, char **restrict, int);

int rand (void);
void srand (unsigned);

void *malloc (size_t);
void *calloc (size_t, size_t);
void *realloc (void *, size_t);
void free (void *);
void *aligned_alloc(size_t, size_t);

_Noreturn void abort (void);
int atexit (void (*) (void));
_Noreturn void exit (int);
_Noreturn void _Exit (int);
int at_quick_exit (void (*) (void));
_Noreturn void quick_exit (int);

char *getenv (const char *);

int system (const char *);

void *bsearch (const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
void qsort (void *, size_t, size_t, int (*)(const void *, const void *));

int abs (int);
long labs (long);
long long llabs (long long);

typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t div (int, int);
ldiv_t ldiv (long, long);
lldiv_t lldiv (long long, long long);

int mblen (const char *, size_t);
int mbtowc (wchar_t *restrict, const char *restrict, size_t);
int wctomb (char *, wchar_t);
size_t mbstowcs (wchar_t *restrict, const char *restrict, size_t);
size_t wcstombs (char *restrict, const wchar_t *restrict, size_t);




size_t __ctype_get_mb_cur_max(void);
# 99 "/usr/include//stdlib.h" 3
int posix_memalign (void **, size_t, size_t);
int setenv (const char *, const char *, int);
int unsetenv (const char *);
int mkstemp (char *);
int mkostemp (char *, int);
char *mkdtemp (char *);
int getsubopt (char **, char *const *, char **);
int rand_r (unsigned *);






char *realpath (const char *restrict, char *restrict);
long int random (void);
void srandom (unsigned int);
char *initstate (unsigned int, char *, size_t);
char *setstate (char *);
int putenv (char *);
int posix_openpt (int);
int grantpt (int);
int unlockpt (int);
char *ptsname (int);
char *l64a (long);
long a64l (const char *);
void setkey (const char *);
double drand48 (void);
double erand48 (unsigned short [3]);
long int lrand48 (void);
long int nrand48 (unsigned short [3]);
long mrand48 (void);
long jrand48 (unsigned short [3]);
void srand48 (long);
unsigned short *seed48 (unsigned short [3]);
void lcong48 (unsigned short [7]);
# 1 "/usr/include//alloca.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 9 "/usr/include//alloca.h" 3


void *alloca(size_t);
# 138 "/usr/include//stdlib.h" 3

char *mktemp (char *);
int mkstemps (char *, int);
int mkostemps (char *, int, int);
void *valloc (size_t);
void *memalign(size_t, size_t);
int getloadavg(double *, int);
int clearenv(void);
# 24 "aacquant.c"

# 1 "frame.h"

# 1 "../config.h"
 
# 26 "frame.h"

# 1 "/usr/include//sys/types.h" 3

# 1 "/usr/include//features.h" 3

# 7 "/usr/include//sys/types.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 55 "/usr/include//bits/alltypes.h" 3
typedef long time_t;




typedef long suseconds_t;





typedef struct { union { int __i[14]; volatile int __vi[14]; unsigned long __s[7]; } __u; } pthread_attr_t;




typedef struct { union { int __i[10]; volatile int __vi[10]; volatile void *volatile __p[5]; } __u; } pthread_mutex_t;
# 81 "/usr/include//bits/alltypes.h" 3
typedef struct { union { int __i[12]; volatile int __vi[12]; void *__p[6]; } __u; } pthread_cond_t;
# 91 "/usr/include//bits/alltypes.h" 3
typedef struct { union { int __i[14]; volatile int __vi[14]; void *__p[7]; } __u; } pthread_rwlock_t;




typedef struct { union { int __i[8]; volatile int __vi[8]; void *__p[4]; } __u; } pthread_barrier_t;
# 116 "/usr/include//bits/alltypes.h" 3
typedef long ssize_t;
# 131 "/usr/include//bits/alltypes.h" 3
typedef long register_t;





typedef signed char     int8_t;




typedef short           int16_t;




typedef int             int32_t;




typedef long          int64_t;
# 182 "/usr/include//bits/alltypes.h" 3
typedef unsigned long u_int64_t;
# 193 "/usr/include//bits/alltypes.h" 3
typedef unsigned mode_t;




typedef unsigned long nlink_t;




typedef long off_t;




typedef unsigned long ino_t;




typedef unsigned long dev_t;




typedef long blksize_t;




typedef long blkcnt_t;




typedef unsigned long fsblkcnt_t;




typedef unsigned long fsfilcnt_t;
# 250 "/usr/include//bits/alltypes.h" 3
typedef void * timer_t;




typedef int clockid_t;




typedef long clock_t;
# 276 "/usr/include//bits/alltypes.h" 3
typedef int pid_t;




typedef unsigned id_t;




typedef unsigned uid_t;




typedef unsigned gid_t;




typedef int key_t;




typedef unsigned useconds_t;
# 314 "/usr/include//bits/alltypes.h" 3
typedef struct __pthread * pthread_t;





typedef int pthread_once_t;




typedef unsigned pthread_key_t;




typedef int pthread_spinlock_t;




typedef struct { unsigned __attr; } pthread_mutexattr_t;




typedef struct { unsigned __attr; } pthread_condattr_t;




typedef struct { unsigned __attr; } pthread_barrierattr_t;




typedef struct { unsigned __attr[2]; } pthread_rwlockattr_t;
# 57 "/usr/include//sys/types.h" 3



typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned u_int32_t;
typedef char *caddr_t;
typedef unsigned char u_char;
typedef unsigned short u_short, ushort;
typedef unsigned u_int, uint;
typedef unsigned long u_long, ulong;
typedef long long quad_t;
typedef unsigned long long u_quad_t;
# 1 "/usr/include//endian.h" 3

# 1 "/usr/include//features.h" 3

# 4 "/usr/include//endian.h" 3

# 1 "/usr/include//stdint.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 106 "/usr/include//bits/alltypes.h" 3
typedef unsigned long uintptr_t;
# 121 "/usr/include//bits/alltypes.h" 3
typedef long intptr_t;
# 157 "/usr/include//bits/alltypes.h" 3
typedef long          intmax_t;




typedef unsigned char   uint8_t;




typedef unsigned short  uint16_t;




typedef unsigned int    uint32_t;




typedef unsigned long uint64_t;
# 187 "/usr/include//bits/alltypes.h" 3
typedef unsigned long uintmax_t;
# 20 "/usr/include//stdint.h" 3


typedef int8_t int_fast8_t;
typedef int64_t int_fast64_t;

typedef int8_t  int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_fast8_t;
typedef uint64_t uint_fast64_t;

typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;
# 1 "/usr/include//bits/stdint.h" 3
typedef int32_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
# 95 "/usr/include//stdint.h" 3

# 23 "/usr/include//endian.h" 3


static inline uint16_t __bswap16(uint16_t __x)
{
return __x<<8 | __x>>8;
}

static inline uint32_t __bswap32(uint32_t __x)
{
return __x>>24 | __x>>8&0xff00 | __x<<8&0xff0000 | __x<<24;
}

static inline uint64_t __bswap64(uint64_t __x)
{
return __bswap32(__x)+0ULL<<32 | __bswap32(__x>>32);
}
# 70 "/usr/include//sys/types.h" 3

# 1 "/usr/include//sys/select.h" 3

# 1 "/usr/include//features.h" 3

# 7 "/usr/include//sys/select.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 265 "/usr/include//bits/alltypes.h" 3
struct timeval { time_t tv_sec; suseconds_t tv_usec; };




struct timespec { time_t tv_sec; long tv_nsec; };
# 374 "/usr/include//bits/alltypes.h" 3
typedef struct __sigset_t { unsigned long __bits[128/sizeof(long)]; } sigset_t;
# 16 "/usr/include//sys/select.h" 3




typedef unsigned long fd_mask;

typedef struct {
unsigned long fds_bits[1024 / 8 / sizeof(long)];
} fd_set;






int select (int, fd_set *restrict, fd_set *restrict, fd_set *restrict, struct timeval *restrict);
int pselect (int, fd_set *restrict, fd_set *restrict, fd_set *restrict, const struct timespec *restrict, const sigset_t *restrict);
# 71 "/usr/include//sys/types.h" 3

# 1 "/usr/include//sys/sysmacros.h" 3

# 72 "/usr/include//sys/types.h" 3

# 30 "frame.h"

# 1 "/usr/include//inttypes.h" 3

# 1 "/usr/include//features.h" 3

# 8 "/usr/include//inttypes.h" 3

# 1 "/usr/include//stdint.h" 3

# 9 "/usr/include//inttypes.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 12 "/usr/include//inttypes.h" 3


typedef struct { intmax_t quot, rem; } imaxdiv_t;

intmax_t imaxabs(intmax_t);
imaxdiv_t imaxdiv(intmax_t, intmax_t);

intmax_t strtoimax(const char *restrict, char **restrict, int);
uintmax_t strtoumax(const char *restrict, char **restrict, int);

intmax_t wcstoimax(const wchar_t *restrict, wchar_t **restrict, int);
uintmax_t wcstoumax(const wchar_t *restrict, wchar_t **restrict, int);
# 33 "frame.h"

# 1 "/usr/include//stdint.h" 3

# 36 "frame.h"

# 1 "coder.h"

# 58 "coder.h"
enum WINDOW_TYPE {
ONLY_LONG_WINDOW,
LONG_SHORT_WINDOW,
ONLY_SHORT_WINDOW,
SHORT_LONG_WINDOW
};
# 91 "coder.h"
typedef struct {
int order;                            
int direction;                        
int coefCompress;                     
int length;                           
double aCoeffs[20+1];      
double kCoeffs[20+1];      
int index[20+1];           
} TnsFilterData;

typedef struct {
int numFilters;                              
int coefResolution;                          
TnsFilterData tnsFilter[1<<2];  
} TnsWindowData;

typedef struct {
int tnsDataPresent;
int tnsMinBandNumberLong;
int tnsMinBandNumberShort;
int tnsMaxBandsLong;
int tnsMaxBandsShort;
int tnsMaxOrderLong;
int tnsMaxOrderShort;
TnsWindowData windowData[8];  
} TnsInfo;

typedef struct
{
int weight_idx;
double weight;
int sbk_prediction_used[8];
int sfb_prediction_used[((15+1)*8)];
int delay[8];
int global_pred_flag;
int side_info;
double *buffer;
double *mdct_predicted;

double *time_buffer;
double *ltp_overlap_buffer;
} LtpInfo;

typedef struct
{
int psy_init_mc;
double dr_mc[2][1024],e_mc[2+1+1][1024];
double K_mc[2+1][1024], R_mc[2+1][1024];
double VAR_mc[2+1][1024], KOR_mc[2+1][1024];
double sb_samples_pred_mc[1024];
int thisLineNeedsResetting_mc[1024];
int reset_count_mc;
} BwpInfo;


typedef struct {
int window_shape;
int prev_window_shape;
int block_type;
int desired_block_type;

int global_gain;
int scale_factor[((15+1)*8)];

int num_window_groups;
int window_group_length[8];
int max_sfb;
int nr_of_sfb;
int sfb_offset[250];
int lastx;
double avgenrg;

int spectral_count;


int book_vector[((15+1)*8)];



int *data;


int *len;
# 185 "coder.h"
double *requantFreq;

TnsInfo tnsInfo;
LtpInfo ltpInfo;
BwpInfo bwpInfo;

int max_pred_sfb;
int pred_global_flag;
int pred_sfb_flag[((15+1)*8)];
int reset_group_number;

} CoderInfo;

typedef struct {
unsigned long sampling_rate;   
int num_cb_long;
int num_cb_short;
int cb_width_long[51];
int cb_width_short[15];
} SR_INFO;
# 47 "frame.h"

# 1 "channels.h"

# 1 "coder.h"

# 29 "channels.h"


typedef struct {
int is_present;
int ms_used[((15+1)*8)];
} MSInfo;

typedef struct {
int tag;
int present;
int ch_is_left;
int paired_ch;
int common_window;
int cpe;
int sce;
int lfe;
MSInfo msInfo;
} ChannelInfo;

void GetChannelInfo(ChannelInfo *channelInfo, int numChannels, int useLfe);
# 48 "frame.h"

# 1 "psych.h"

# 1 "coder.h"

# 33 "psych.h"

# 1 "channels.h"

# 34 "psych.h"

# 1 "fft.h"

# 25 "fft.h"
typedef float fftfloat;
# 39 "fft.h"
typedef struct
{
fftfloat **costbl;
fftfloat **negsintbl;
unsigned short **reordertbl;
} FFT_Tables;



void fft_initialize		( FFT_Tables *fft_tables );
void fft_terminate	( FFT_Tables *fft_tables );

void rfft			( FFT_Tables *fft_tables, double *x, int logm );
void fft			( FFT_Tables *fft_tables, double *xr, double *xi, int logm );
void ffti			( FFT_Tables *fft_tables, double *xr, double *xi, int logm );
# 35 "psych.h"


typedef struct {
int size;
int sizeS;


double *prevSamples;
double *prevSamplesS;

int block_type;

void *data;
} PsyInfo;

typedef struct {
double sampleRate;


double *hannWindow;
double *hannWindowS;

void *data;
} GlobalPsyInfo;

typedef struct 
{
void (*PsyInit) (GlobalPsyInfo *gpsyInfo, PsyInfo *psyInfo,
unsigned int numChannels, unsigned int sampleRate,
int *cb_width_long, int num_cb_long,
int *cb_width_short, int num_cb_short);
void (*PsyEnd) (GlobalPsyInfo *gpsyInfo, PsyInfo *psyInfo,
unsigned int numChannels);
void (*PsyCalculate) (ChannelInfo *channelInfo, GlobalPsyInfo *gpsyInfo,
PsyInfo *psyInfo, int *cb_width_long, int num_cb_long,
int *cb_width_short, int num_cb_short,
unsigned int numChannels);
void (*PsyBufferUpdate) ( FFT_Tables *fft_tables, GlobalPsyInfo * gpsyInfo, PsyInfo * psyInfo,
double *newSamples, unsigned int bandwidth,
int *cb_width_short, int num_cb_short);
void (*BlockSwitch) (CoderInfo *coderInfo, PsyInfo *psyInfo,
unsigned int numChannels);
} psymodel_t;

extern psymodel_t psymodel2;
# 49 "frame.h"

# 1 "aacquant.h"

# 1 "coder.h"

# 29 "aacquant.h"

# 1 "psych.h"

# 30 "aacquant.h"

# 40 "aacquant.h"

#pragma pack(push, 1)
# 40 "aacquant.h"

typedef struct
{
double *pow43;
double *adj43;
double quality;
} AACQuantCfg;

#pragma pack(pop)
# 47 "aacquant.h"


void AACQuantizeInit(CoderInfo *coderInfo, unsigned int numChannels,
AACQuantCfg *aacquantCfg);
void AACQuantizeEnd(CoderInfo *coderInfo, unsigned int numChannels,
AACQuantCfg *aacquantCfg);

int AACQuantize(CoderInfo *coderInfo,
PsyInfo *psyInfo,
ChannelInfo *channelInfo,
int *cb_width,
int num_cb,
double *xr,
AACQuantCfg *aacquantcfg);

int SortForGrouping(CoderInfo* coderInfo,
PsyInfo *psyInfo,
ChannelInfo *channelInfo,
int *sfb_width_table,
double *xr);
void CalcAvgEnrg(CoderInfo *coderInfo,
const double *xr);
# 50 "frame.h"

# 1 "fft.h"

# 51 "frame.h"

# 63 "frame.h"

#pragma pack(push, 1)
# 63 "frame.h"


typedef struct {
psymodel_t *model;
char *name;
} psymodellist_t;
# 1 "../include/faaccfg.h"

# 48 "../include/faaccfg.h"

#pragma pack(push, 1)
# 48 "../include/faaccfg.h"

typedef struct faacEncConfiguration
{

int version;


char *name;


char *copyright;


unsigned int mpegVersion;


unsigned int aacObjectType;


unsigned int allowMidside;


unsigned int useLfe;


unsigned int useTns;


unsigned long bitRate;


unsigned int bandWidth;


unsigned long quantqual;


unsigned int outputFormat;


psymodellist_t *psymodellist;


unsigned int psymodelidx;
# 101 "../include/faaccfg.h"
unsigned int inputFormat;


int shortctl;
# 116 "../include/faaccfg.h"
int channel_map[64];	

} faacEncConfiguration, *faacEncConfigurationPtr;


#pragma pack(pop)
# 120 "../include/faaccfg.h"

# 70 "frame.h"


typedef struct {

unsigned int numChannels;


unsigned long sampleRate;
unsigned int sampleRateIdx;

unsigned int usedBytes;


unsigned int frameNum;
unsigned int flushFrame;


SR_INFO *srInfo;


double *sampleBuff[64];
double *nextSampleBuff[64];
double *next2SampleBuff[64];
double *next3SampleBuff[64];
double *ltpTimeBuff[64];


double *sin_window_long;
double *sin_window_short;
double *kbd_window_long;
double *kbd_window_short;
double *freqBuff[64];
double *overlapBuff[64];

double *msSpectrum[64];


CoderInfo coderInfo[64];
ChannelInfo channelInfo[64];


PsyInfo psyInfo[64];
GlobalPsyInfo gpsyInfo;


faacEncConfiguration config;

psymodel_t *psymodel;


AACQuantCfg aacquantCfg;


FFT_Tables	fft_tables;


int bitDiff;
} faacEncStruct, *faacEncHandle;

int  faacEncGetVersion(char **faac_id_string,
char **faac_copyright_string);

int  faacEncGetDecoderSpecificInfo(faacEncHandle hEncoder,
unsigned char** ppBuffer,
unsigned long* pSizeOfDecoderSpecificInfo);

faacEncConfigurationPtr  faacEncGetCurrentConfiguration(faacEncHandle hEncoder);
int  faacEncSetConfiguration (faacEncHandle hEncoder, faacEncConfigurationPtr config);

faacEncHandle  faacEncOpen(unsigned long sampleRate,
unsigned int numChannels,
unsigned long *inputSamples,
unsigned long *maxOutputBytes);

int  faacEncEncode(faacEncHandle hEncoder,
int32_t *inputBuffer,
unsigned int samplesInput,
unsigned char *outputBuffer,
unsigned int bufferSize
);

int  faacEncClose(faacEncHandle hEncoder);



#pragma pack(pop)
# 154 "frame.h"

# 26 "aacquant.c"

# 1 "aacquant.h"

# 27 "aacquant.c"

# 1 "coder.h"

# 28 "aacquant.c"

# 1 "huffman.h"

# 1 "bitstream.h"

# 1 "frame.h"

# 45 "bitstream.h"

# 1 "coder.h"

# 46 "bitstream.h"

# 1 "channels.h"

# 47 "bitstream.h"

# 141 "bitstream.h"
typedef struct
{
unsigned char *data;       
long numBit;           
long size;             
long currentBit;       
long numByte;          
} BitStream;



int WriteBitstream(faacEncHandle hEncoder,
CoderInfo *coderInfo,
ChannelInfo *channelInfo,
BitStream *bitStream,
int numChannels);


BitStream *OpenBitStream(int size, unsigned char *buffer);

int CloseBitStream(BitStream *bitStream);

int PutBit(BitStream *bitStream,
unsigned long data,
int numBit);
# 35 "huffman.h"

# 1 "coder.h"

# 36 "huffman.h"

# 1 "frame.h"

# 50 "huffman.h"


void HuffmanInit(CoderInfo *coderInfo, unsigned int numChannels);
void HuffmanEnd(CoderInfo *coderInfo, unsigned int numChannels);

int BitSearch(CoderInfo *coderInfo,
int *quant);

int NoiselessBitCount(CoderInfo *coderInfo,
int *quant,
int hop,
int min_book_choice[112][3]);

static int CalculateEscSequence(int input, int *len_esc_sequence);

int CalcBits(CoderInfo *coderInfo,
int book,
int *quant,
int offset,
int length);

int OutputBits(CoderInfo *coderInfo,



int book,

int *quant,
int offset,
int length);

int SortBookNumbers(CoderInfo *coderInfo,
BitStream *bitStream,
int writeFlag);

int WriteScalefactors(CoderInfo *coderInfo,
BitStream *bitStream,
int writeFlag);
# 29 "aacquant.c"

# 1 "psych.h"

# 30 "aacquant.c"

# 1 "util.h"

# 1 "/usr/include//stdlib.h" 3

# 29 "util.h"

# 1 "/usr/include//memory.h" 3

# 1 "/usr/include//string.h" 3

# 1 "/usr/include//features.h" 3

# 8 "/usr/include//string.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 368 "/usr/include//bits/alltypes.h" 3
typedef struct __locale_struct * locale_t;
# 23 "/usr/include//string.h" 3


void *memcpy (void *restrict, const void *restrict, size_t);
void *memmove (void *, const void *, size_t);
void *memset (void *, int, size_t);
int memcmp (const void *, const void *, size_t);
void *memchr (const void *, int, size_t);

char *strcpy (char *restrict, const char *restrict);
char *strncpy (char *restrict, const char *restrict, size_t);

char *strcat (char *restrict, const char *restrict);
char *strncat (char *restrict, const char *restrict, size_t);

int strcmp (const char *, const char *);
int strncmp (const char *, const char *, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *restrict, const char *restrict, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strstr (const char *, const char *);
char *strtok (char *restrict, const char *restrict);

size_t strlen (const char *);

char *strerror (int);
# 1 "/usr/include//strings.h" 3

# 1 "/usr/include//bits/alltypes.h" 3

# 11 "/usr/include//strings.h" 3





int bcmp (const void *, const void *, size_t);
void bcopy (const void *, void *, size_t);
void bzero (void *, size_t);
char *index (const char *, int);
char *rindex (const char *, int);



int ffs (int);
int ffsl (long);
int ffsll (long long);


int strcasecmp (const char *, const char *);
int strncasecmp (const char *, const char *, size_t);

int strcasecmp_l (const char *, const char *, locale_t);
int strncasecmp_l (const char *, const char *, size_t, locale_t);
# 57 "/usr/include//string.h" 3






char *strtok_r (char *restrict, const char *restrict, char **restrict);
int strerror_r (int, char *, size_t);
char *stpcpy(char *restrict, const char *restrict);
char *stpncpy(char *restrict, const char *restrict, size_t);
size_t strnlen (const char *, size_t);
char *strdup (const char *);
char *strndup (const char *, size_t);
char *strsignal(int);
char *strerror_l (int, locale_t);
int strcoll_l (const char *, const char *, locale_t);
size_t strxfrm_l (char *restrict, const char *restrict, size_t, locale_t);




void *memccpy (void *restrict, const void *restrict, int, size_t);



char *strsep(char **, const char *);
size_t strlcat (char *, const char *, size_t);
size_t strlcpy (char *, const char *, size_t);
# 1 "/usr/include//memory.h" 3

# 30 "util.h"

# 48 "util.h"
int GetSRIndex(unsigned int sampleRate);
int GetMaxPredSfb(int samplingRateIdx);
unsigned int MaxBitrate(unsigned long sampleRate);
unsigned int MinBitrate();
unsigned int MaxBitresSize(unsigned long bitRate, unsigned long sampleRate);
unsigned int BitAllocation(double pe, int short_block);
# 31 "aacquant.c"








static int FixNoise(CoderInfo *coderInfo,
const double *xr,
double *xr_pow,
int *xi,
double *xmin,
double *pow43,
double *adj43);

static void CalcAllowedDist(CoderInfo *coderInfo, PsyInfo *psyInfo,
double *xr, double *xmin, int quality);


void AACQuantizeInit(CoderInfo *coderInfo, unsigned int numChannels,
AACQuantCfg *aacquantCfg)
{
unsigned int channel, i;

aacquantCfg->pow43 = (double*)malloc((8191+2)*sizeof(double));
aacquantCfg->adj43 = (double*)malloc((8191+2)*sizeof(double));

aacquantCfg->pow43[0] = 0.0;
for(i=1;i<(8191+2);i++)
aacquantCfg->pow43[i] = pow((double)i, 4.0/3.0);


aacquantCfg->adj43[0] = 0.0;
for (i = 1; i < (8191+2); i++)
aacquantCfg->adj43[i] = i - 0.5 - pow(0.5 * (aacquantCfg->pow43[i - 1] + aacquantCfg->pow43[i]),0.75);






for (channel = 0; channel < numChannels; channel++) {
coderInfo[channel].requantFreq = (double*)malloc(1024*sizeof(double));
}
}

void AACQuantizeEnd(CoderInfo *coderInfo, unsigned int numChannels,
AACQuantCfg *aacquantCfg)
{
unsigned int channel;

if (aacquantCfg->pow43)
{
free(aacquantCfg->pow43);
aacquantCfg->pow43 = ((void*)0);
}
if (aacquantCfg->adj43)
{
free(aacquantCfg->adj43);
aacquantCfg->adj43 = ((void*)0);
}

for (channel = 0; channel < numChannels; channel++) {
if (coderInfo[channel].requantFreq) free(coderInfo[channel].requantFreq);
}
}

static void BalanceEnergy(CoderInfo *coderInfo,
const double *xr, const int *xi,
double *pow43)
{
const double ifqstep = pow(2.0, 0.25);
const double logstep_1 = 1.0 / log(ifqstep);
int sb;
int nsfb = coderInfo->nr_of_sfb;
int start, end;
int l;
double en0, enq;
int shift;

for (sb = 0; sb < nsfb; sb++)
{
double qfac_1;

start = coderInfo->sfb_offset[sb];
end   = coderInfo->sfb_offset[sb+1];

qfac_1 = pow(2.0, -0.25*(coderInfo->scale_factor[sb] - coderInfo->global_gain));

en0 = 0.0;
enq = 0.0;
for (l = start; l < end; l++)
{
double xq;

if (!sb && !xi[l])
continue;

xq = pow43[xi[l]];

en0 += xr[l] * xr[l];
enq += xq * xq;
}

if (enq == 0.0)
continue;

enq *= qfac_1 * qfac_1;

shift = (int)(log(sqrt(enq / en0)) * logstep_1 + 1000.5);
shift -= 1000;

shift += coderInfo->scale_factor[sb];
coderInfo->scale_factor[sb] = shift;
}
}

static void UpdateRequant(CoderInfo *coderInfo, int *xi,
double *pow43)
{
double *requant_xr = coderInfo->requantFreq;
int sb;
int i;

for (sb = 0; sb < coderInfo->nr_of_sfb; sb++)
{
double invQuantFac =
pow(2.0, -0.25*(coderInfo->scale_factor[sb] - coderInfo->global_gain));
int start = coderInfo->sfb_offset[sb];
int end = coderInfo->sfb_offset[sb + 1];

for (i = start; i < end; i++)
requant_xr[i] = pow43[xi[i]] * invQuantFac;
}
}

int AACQuantize(CoderInfo *coderInfo,
PsyInfo *psyInfo,
ChannelInfo *channelInfo,
int *cb_width,
int num_cb,
double *xr,
AACQuantCfg *aacquantCfg)
{
int sb, i, do_q = 0;
int bits = 0, sign;
double xr_pow[1024];
double xmin[((15+1)*8)];
int xi[1024];


int *scale_factor = coderInfo->scale_factor;


coderInfo->global_gain = 0;
for (sb = 0; sb < coderInfo->nr_of_sfb; sb++)
scale_factor[sb] = 0;


for (i = 0; i < 1024; i++) {
double temp = fabs(xr[i]);
xr_pow[i] = sqrt(temp * sqrt(temp));
do_q += (temp > 1E-20);
}

if (do_q) {
CalcAllowedDist(coderInfo, psyInfo, xr, xmin, aacquantCfg->quality);
coderInfo->global_gain = 0;
FixNoise(coderInfo, xr, xr_pow, xi, xmin,
aacquantCfg->pow43, aacquantCfg->adj43);
BalanceEnergy(coderInfo, xr, xi, aacquantCfg->pow43);
UpdateRequant(coderInfo, xi, aacquantCfg->pow43);

for ( i = 0; i < 1024; i++ )  {
sign = (xr[i] < 0) ? -1 : 1;
xi[i] *= sign;
coderInfo->requantFreq[i] *= sign;
}
} else {
coderInfo->global_gain = 0;
memset(xi, 0, 1024*sizeof(int));
}

BitSearch(coderInfo, xi);


for (i = 0; i < coderInfo->nr_of_sfb; i++) {
if ((coderInfo->book_vector[i]!=15)&&(coderInfo->book_vector[i]!=14)) {
scale_factor[i] = coderInfo->global_gain - scale_factor[i] + 100;
}
}
coderInfo->global_gain = scale_factor[0];






{
int previous_scale_factor = coderInfo->global_gain;
int previous_is_factor = 0;
for (i = 0; i < coderInfo->nr_of_sfb; i++) {
if ((coderInfo->book_vector[i]==15) ||
(coderInfo->book_vector[i]==14)) {
const int diff = scale_factor[i] - previous_is_factor;
if (diff < -60) scale_factor[i] = previous_is_factor - 60;
else if (diff > 59) scale_factor[i] = previous_is_factor + 59;
previous_is_factor = scale_factor[i];

} else if (coderInfo->book_vector[i]) {
const int diff = scale_factor[i] - previous_scale_factor;
if (diff < -60) scale_factor[i] = previous_scale_factor - 60;
else if (diff > 59) scale_factor[i] = previous_scale_factor + 59;
previous_scale_factor = scale_factor[i];

}
}
}








coderInfo->spectral_count = 0;
sb = 0;
for(i = 0; i < coderInfo->nr_of_sfb; i++) {
OutputBits(
coderInfo,



coderInfo->book_vector[i],

xi,
coderInfo->sfb_offset[i],
coderInfo->sfb_offset[i+1]-coderInfo->sfb_offset[i]);

if (coderInfo->book_vector[i])
sb = i;
}


coderInfo->max_sfb = coderInfo->nr_of_sfb = sb + 1;

return bits;
}




typedef union {
float f;
int i;
} fi_union;
# 325 "aacquant.c"
static void QuantizeBand(const double *xp, int *pi, double istep,
int offset, int end, double *adj43)
{
int j;
fi_union *fi;

fi = (fi_union *)pi;
for (j = offset; j < end; j++)
{
double x0 = istep * xp[j];

x0 += (65536*(128)); fi[j].f = (float)x0;
fi[j].f = x0 + (adj43 - 0x4b000000)[fi[j].i];
fi[j].i -= 0x4b000000;
}
}
# 400 "aacquant.c"
static void CalcAllowedDist(CoderInfo *coderInfo, PsyInfo *psyInfo,
double *xr, double *xmin, int quality)
{
int sfb, start, end, l;
const double globalthr = 132.0 / (double)quality;
int last = coderInfo->lastx;
int lastsb = 0;
int *cb_offset = coderInfo->sfb_offset;
int num_cb = coderInfo->nr_of_sfb;
double avgenrg = coderInfo->avgenrg;

for (sfb = 0; sfb < num_cb; sfb++)
{
if (last > cb_offset[sfb])
lastsb = sfb;
}

for (sfb = 0; sfb < num_cb; sfb++)
{
double thr, tmp;
double enrg = 0.0;

start = cb_offset[sfb];
end = cb_offset[sfb + 1];

if (sfb > lastsb)
{
xmin[sfb] = 0;
continue;
}

if (coderInfo->block_type != ONLY_SHORT_WINDOW)
{
double enmax = -1.0;
double lmax;

lmax = start;
for (l = start; l < end; l++)
{
if (enmax < (xr[l] * xr[l]))
{
enmax = xr[l] * xr[l];
lmax = l;
}
}

start = lmax - 2;
end = lmax + 3;
if (start < 0)
start = 0;
if (end > last)
end = last;
}

for (l = start; l < end; l++)
{
enrg += xr[l]*xr[l];
}

thr = enrg/((double)(end-start)*avgenrg);
thr = pow(thr, 0.1*(lastsb-sfb)/lastsb + 0.3);

tmp = 1.0 - ((double)start / (double)last);
tmp = tmp * tmp * tmp + 0.075;

thr = 1.0 / (1.4*thr + tmp);

xmin[sfb] = ((coderInfo->block_type == ONLY_SHORT_WINDOW) ? 0.65 : 1.12)
* globalthr * thr;
}
}

static int FixNoise(CoderInfo *coderInfo,
const double *xr,
double *xr_pow,
int *xi,
double *xmin,
double *pow43,
double *adj43)
{
int i, sb;
int start, end;
double diffvol;
double tmp;
const double ifqstep = pow(2.0, 0.1875);
const double log_ifqstep = 1.0 / log(ifqstep);
const double maxstep = 0.05;

for (sb = 0; sb < coderInfo->nr_of_sfb; sb++)
{
double sfacfix;
double fixstep = 0.25;
int sfac;
double fac;
int dist;
double sfacfix0 = 1.0, dist0 = 1e50;
double maxx;

start = coderInfo->sfb_offset[sb];
end = coderInfo->sfb_offset[sb+1];

if (!xmin[sb])
goto nullsfb;

maxx = 0.0;
for (i = start; i < end; i++)
{
if (xr_pow[i] > maxx)
maxx = xr_pow[i];
}


if (maxx < 10.0)
{
nullsfb:
for (i = start; i < end; i++)
xi[i] = 0;
coderInfo->scale_factor[sb] = 10;
continue;
}

sfacfix = 1.0 / maxx;
sfac = (int)(log(sfacfix) * log_ifqstep - 0.5);
for (i = start; i < end; i++)
xr_pow[i] *= sfacfix;
maxx *= sfacfix;
coderInfo->scale_factor[sb] = sfac;
QuantizeBand(xr_pow, xi, pow(2.0,-((double)coderInfo->global_gain)*.1875), start, end,
adj43);


calcdist:
diffvol = 0.0;
for (i = start; i < end; i++)
{
tmp = xi[i];
diffvol += tmp * tmp;   
}

if (diffvol < 1e-6)
diffvol = 1e-6;
tmp = pow(diffvol / (double)(end - start), -0.666);

if (fabs(fixstep) > maxstep)
{
double dd = 0.5*(tmp / xmin[sb] - 1.0);

if (fabs(dd) < fabs(fixstep))
{
fixstep = dd;

if (fabs(fixstep) < maxstep)
fixstep = maxstep * ((fixstep > 0) ? 1 : -1);
}
}

if (fixstep > 0)
{
if (tmp < dist0)
{
dist0 = tmp;
sfacfix0 = sfacfix;
}
else
{
if (fixstep > .1)
fixstep = .1;
}
}
else
{
dist0 = tmp;
sfacfix0 = sfacfix;
}

dist = (tmp > xmin[sb]);
fac = 0.0;
if (fabs(fixstep) >= maxstep)
{
if ((dist && (fixstep < 0))
|| (!dist && (fixstep > 0)))
{
fixstep = -0.5 * fixstep;
}

fac = 1.0 + fixstep;
}
else if (dist)
{
fac = 1.0 + fabs(fixstep);
}

if (fac != 0.0)
{
if (maxx * fac >= 8191)
{

fac = sfacfix0 / sfacfix;
for (i = start; i < end; i++)
xr_pow[i] *= fac;
maxx *= fac;
sfacfix *= fac;
coderInfo->scale_factor[sb] = log(sfacfix) * log_ifqstep - 0.5;
QuantizeBand(xr_pow, xi, pow(2.0,-((double)coderInfo->global_gain)*.1875), start, end,
adj43);
continue;
}

if (coderInfo->scale_factor[sb] < -10)
{
for (i = start; i < end; i++)
xr_pow[i] *= fac;
maxx *= fac;
sfacfix *= fac;
coderInfo->scale_factor[sb] = log(sfacfix) * log_ifqstep - 0.5;
QuantizeBand(xr_pow, xi, pow(2.0,-((double)coderInfo->global_gain)*.1875), start, end,
adj43);
goto calcdist;
}
}
}
return 0;
}

int SortForGrouping(CoderInfo* coderInfo,
PsyInfo *psyInfo,
ChannelInfo *channelInfo,
int *sfb_width_table,
double *xr)
{
int i,j,ii;
int index = 0;
double xr_tmp[1024];
int group_offset=0;
int k=0;
int windowOffset = 0;



int* sfb_offset = coderInfo->sfb_offset;
int* nr_of_sfb = &(coderInfo->nr_of_sfb);
int* window_group_length;
int num_window_groups;
*nr_of_sfb = coderInfo->max_sfb;               
window_group_length = coderInfo->window_group_length;
num_window_groups = coderInfo->num_window_groups;


sfb_offset[k]=0;
for (k=1 ; k <*nr_of_sfb+1; k++) {
sfb_offset[k] = sfb_offset[k-1] + sfb_width_table[k-1];
}


index = 0;
group_offset=0;
for (i=0; i< num_window_groups; i++) {
for (k=0; k<*nr_of_sfb; k++) {
for (j=0; j < window_group_length[i]; j++) {
for (ii=0;ii< sfb_width_table[k];ii++)
xr_tmp[index++] = xr[ii+ sfb_offset[k] + 128*j +group_offset];
}
}
group_offset +=  128*window_group_length[i];
}

for (k=0; k<1024; k++){
xr[k] = xr_tmp[k];
}



index = 0;
sfb_offset[index++] = 0;
windowOffset = 0;
for (i=0; i < num_window_groups; i++) {
for (k=0 ; k <*nr_of_sfb; k++) {
sfb_offset[index] = sfb_offset[index-1] + sfb_width_table[k]*window_group_length[i] ;
index++;
}
windowOffset += window_group_length[i];
}

*nr_of_sfb = *nr_of_sfb * num_window_groups;   

return 0;
}

void CalcAvgEnrg(CoderInfo *coderInfo,
const double *xr)
{
int end, l;
int last = 0;
double totenrg = 0.0;

end = coderInfo->sfb_offset[coderInfo->nr_of_sfb];
for (l = 0; l < end; l++)
{
if (xr[l])
{
last = l;
totenrg += xr[l] * xr[l];
}
}
last++;

coderInfo->lastx = last;
coderInfo->avgenrg = totenrg / last;
}
