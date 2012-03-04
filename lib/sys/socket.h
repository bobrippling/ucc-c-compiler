typedef uint32_t socklen_t;

int socket(int domain, int type, int proto);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
