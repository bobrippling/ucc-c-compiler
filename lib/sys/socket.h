#ifndef __SOCKET_H
#define __SOCKET_H

enum
{
	PF_UNSPEC = 0,
	PF_LOCAL  = 1,
	PF_UNIX   = PF_LOCAL,
	PF_FILE   = PF_LOCAL,
	PF_INET   = 2,
};

enum
{
	AF_UNSPEC = PF_UNSPEC,
	AF_LOCAL  = PF_LOCAL,
	AF_UNIX   = PF_UNIX,
	AF_FILE   = PF_FILE,
	AF_INET   = PF_INET,
};

enum
{
	SOCK_STREAM = 1,
	SOCK_DGRAM  = 2,
};

#include "types.h"

typedef uint32_t socklen_t;

struct sockaddr;

int socket(int domain, int type, int proto);
int connect(int fd, const struct sockaddr *addr, socklen_t len);

#endif
