#include <sys/socket.h>
#include <errno.h>

#ifdef MANUAL
struct sockaddr_in
{
	int _in_stuff;             /* 8 bytes */
	unsigned char sin_zero[8]; /* 16 - sizeof above members */
};

int16_t htons(int x)
{
	return ((x & 0xff) << 16) | ((x & 0xff00) >> 16);
}
#else
#  include <arpa/inet.h>
#endif

int main()
{
	struct sockaddr_in addr;
	int sock;

#ifdef MANUAL
#  ifdef MANUAL_PTR
	void *p;
	p = &addr;

	*(short *)p = AF_INET;
	*(short *)(p + 16) = htons(5678);

#  else
	addr._in_stuff = 0;

	addr._in_stuff |= AF_INET     << 14; /* BUH - fix */
	addr._in_stuff |= htons(5678) << 12;
	/* 0.0.0.0 */
#  endif

#else
	addr.sin_port   = htons(5678);
	addr.sin_family = AF_INET;
#endif

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		//perror("socket()");
		return 1;
	}

	switch(connect(sock, &addr, sizeof addr)){
		default: /* FIXME: signed cmp, i think */
		case -1:
			return 5;
			//printf("errno %d\n", errno);
			/*perror("connect()");*/
			//printf("connect(): %s\n", strerror(errno));
			break;
		case 0:
			//printf("connected :)\n");
			break;
	}

	close(sock);

	return 0;
}
