#ifndef TIME_H
#define TIME_H

typedef long time_t, suseconds_t, useconds_t;

struct timeval
{
	time_t      tv_sec;
	suseconds_t tv_usec;
};

struct timespec {
	time_t tv_sec;
	long   tv_nsec;
};

int nanosleep(const struct timespec *req, struct timespec *rem);

#endif
