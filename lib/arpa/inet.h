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
