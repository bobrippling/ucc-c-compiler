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

#ifdef DONT_DEFINE
uint16_t swap16(uint16_t x)
{
	return (((x << 8) & 0xFF00) | ((x >> 8) & 0xFF));
}

uint32_t swap32(uint32_t x)
{
	return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
}
#endif
