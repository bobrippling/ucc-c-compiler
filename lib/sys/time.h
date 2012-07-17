#ifndef TIME_H
#define TIME_H

#ifdef __GOT_SHORT_LONG
typedef long time_t, suseconds_t, useconds_t;
#else
typedef int  time_t, suseconds_t, useconds_t;
#endif


struct timeval
{
	time_t      tv_sec;
	suseconds_t tv_usec;
};

struct timespec {
	time_t tv_sec;
#ifdef __GOT_SHORT_LONG
	long   tv_nsec;
#else
	int tv_nsec;
#endif
};

int nanosleep(const struct timespec *req, struct timespec *rem);

#endif
