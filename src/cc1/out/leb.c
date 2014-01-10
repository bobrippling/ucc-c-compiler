#include <stdio.h>

#include "leb.h"

unsigned leb128_length(unsigned long long value, int is_signed)
{
	unsigned len = 1;

	if(is_signed){
		signed long long sv = value;

		for(;;){
			sv >>= 7;

			if(!sv || sv == -1)
				break;
			len++;
		}
	}else{
		for(;;){
			value >>= 7;

			if(!value) /* more */
				break;
			len++;
		}
	}
	return len;
}

unsigned leb128_out(FILE *f, unsigned long long v, int sig)
{
	const char *join = "";
	unsigned len = 1;

	if(sig){
		signed long long sv = v;

		for(;;){
			unsigned char byte = sv & 0x7f;
			int sign_bit_set = !!(byte & 0x40);
			int done = 0;

			sv >>= 7; /* XXX: assume signed right shift */

			if((sv == 0 && !sign_bit_set)
			|| (sv == -1 && sign_bit_set))
			{
				done = 1;
			}
			else
			{
				byte |= 0x80;
				len++;
			}

			fprintf(f, "%s%d", join, byte);
			join = ", ";

			if(done)
				break;
		}
	}else{
		for(;;){
			unsigned char byte = v & 0x7f;

			v >>= 7;
			if(v){
				/* more */
				byte |= 0x80; /* 0b_1000_0000 */
			}

			fprintf(f, "%s%d", join, byte);
			join = ", ";

			if(byte & 0x80){
				len++;
				continue;
			}
			break;
		}
	}

	return len;
}
