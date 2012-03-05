#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

die()
{
	fprintf(stderr, "dying: %s\n", strerror(errno));
	exit(1);
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct hostent *ent;
	int sock;

	if(argc != 3){
		fprintf(stderr, "Usage: %s host port\n", *argv);
		return 1;
	}

	ent = gethostbyname(argv[1]);

	if(!ent)
		die();

	memset(&addr, 0, sizeof addr);

	memcpy(&addr.sin_addr, ent->h_addr, sizeof addr.sin_addr);

	addr.sin_port = htons(atoi(argv[2]));
	addr.sin_family = AF_INET;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		die();

	if(connect(sock, (struct sockaddr *)&addr, sizeof addr) == 0){
		printf("connected :)\n");

#ifdef EXTRA
		for(;;){
			fd_set fds;

			FD_ZERO(&fds);

			FD_SET(0,    &fds);
			FD_SET(sock, &fds);

			if(select(sock + 1, &fds, NULL, NULL, NULL) == -1)
				die();

			if(FD_ISSET(0, &fds)){
				char buf[256];

				if(fgets(buf, sizeof buf, stdin))
					write(sock, buf, strlen(buf));
			}

			if(FD_ISSET(sock, &fds)){
				char buf[256];
				int n;

				if((n = read(sock, buf, sizeof(buf) - 1)) > 0){
					write(1, buf, n);
				}else if(n == 0){
					printf("closed connection\n");
					break;
				}
			}
		}
#endif
	}

	close(sock);

	printf("bye :(\n");

	return 0;
}
