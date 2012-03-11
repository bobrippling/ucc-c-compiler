#ifdef TYPE_PARSING_TODO
typedef unsigned short int sa_family_t;
typedef uint16_t           in_port_t;
typedef uint32_t           in_addr_t;

struct sockaddr           /* 16 bytes */
{
	sa_family_t sin_family; /*  2 bytes */
	char sa_data[14];       /* 14 bytes */
};

struct in_addr
{
	in_addr_t s_addr;       /* 4 bytes */
};

struct sockaddr_in
{
	sa_family_t sin_family;  /* 2 bytes */
	in_port_t   sin_port;    /* 2 bytes */

	struct in_addr sin_addr; /* 4 bytes */

	unsigned char sin_zero[8]; /* 16 - sizeof above members */
};

/*
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);
*/
uint16_t htons(uint16_t hostshort);
/*
uint16_t ntohs(uint16_t netshort);
*/
#endif
