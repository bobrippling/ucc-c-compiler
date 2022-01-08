#include <stdio.h>

#include "leb.h"

unsigned leb128_length(unsigned long long value, int is_signed)
{
	unsigned len = 1;

	if(is_signed){
		signed long long sv = value;

		for(;;){
			int sign_bit_set = !!(sv & 0x40);

			sv >>= 7;

			if((sv == 0 && !sign_bit_set)
			|| (sv == -1 && sign_bit_set))
			{
				break;
			}

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

unsigned leb128_emit(
		unsigned long long v,
		int is_signed,
		void emit_byte(unsigned char, void *),
		void *ctx)
{
	unsigned len = 1;

	if(is_signed){
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

			emit_byte(byte, ctx);

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

			emit_byte(byte, ctx);

			if(byte & 0x80){
				len++;
				continue;
			}
			break;
		}
	}

	return len;
}

struct emit_ctx
{
	FILE *f;
	const char *join;
};

static void emit_byte(unsigned char byte, void *vctx)
{
	struct emit_ctx *ctx = vctx;
	fprintf(ctx->f, "%s%d", ctx->join, byte);
	ctx->join = ", ";
}

unsigned leb128_out(FILE *f, unsigned long long v, int is_signed)
{
	struct emit_ctx ctx;
	ctx.f = f;
	ctx.join = "";

	return leb128_emit(v, is_signed, emit_byte, &ctx);
}
