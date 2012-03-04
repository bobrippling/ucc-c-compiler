#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
	struct sockaddr_in addr;
	int sock;

	memset(&addr, 0, sizeof addr);

	// TODO: addr.sin_addr

	addr.sin_port   = htons(5678);
	addr.sin_family = AF_INET;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		return 1;

	switch(connect(sock, (struct sockaddr *)&addr, sizeof addr)){
		case -1:
			perror("connect()");
			break;
		case 0:
			printf("connected :)\n");
			break;
		default:
			printf("buf?\n");
	}

	close(sock);

	return 0;
}
