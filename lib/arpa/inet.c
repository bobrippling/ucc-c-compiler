#define TYPE_PARSING_TODO
#include "inet.h"

/*
uint32_t htonl(uint32_t hostlong)
{
}

uint32_t ntohl(uint32_t netlong)
{
}
*/


uint16_t htons(uint16_t hostshort)
{
	/* short is two chars, swap */
	char l, r;
	l = hostshort & 0xff00;
	r = hostshort & 0x00ff;
	return l >> 16 | r << 16;

	//return ((x & 0xff) << 16) | ((x & 0xff00) >> 16);
}

/*
uint16_t ntohs(uint16_t netshort)
{
}
*/
