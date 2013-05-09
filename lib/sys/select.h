#ifndef __SYS_SELECT_H
#define __SYS_SELECT_H

#include "types.h"
#include "time.h"

typedef	struct fd_set
{
	int32_t fds_bits[32];
} fd_set;


void FD_CLR(int fd, fd_set *fdset);
void FD_ZERO(fd_set *fdset);
void FD_SET(int fd, fd_set *fdset);
int  FD_ISSET(int fd, fd_set *fdset);
void FD_COPY(fd_set *fdset_orig, fd_set *fdset_copy);

int select(
		int nfds,
		fd_set *restrict readfds,
		fd_set *restrict writefds,
    fd_set *restrict errorfds,
		struct timeval *restrict timeout);

#endif
