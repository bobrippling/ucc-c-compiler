#include "socket.h"
#include "../syscalls.h"

#ifndef SYS_socket
#  include "socketcall.h"

int socketcall(int call, unsigned long *args)
{
	return __syscall(SYS_socketcall, call, args);
}
#endif

int socket(int domain, int type, int proto)
{
#ifdef SYS_socket
	return __syscall(SYS_socket, domain, type, proto);
#else
	unsigned long args[3];
	args[0] = domain;
	args[1] = type;
	args[2] = proto;
	return socketcall(__socketcall_socket, args);
#endif
}

int connect(int fd, const struct sockaddr *addr, socklen_t len)
{
#ifdef SYS_socket
	return __syscall(SYS_connect, fd, addr, len);
#else
	unsigned long args[3];
	args[0] = fd;
	args[1] = addr;
	args[2] = len;
	return socketcall(__socketcall_connect, args);
#endif
}
