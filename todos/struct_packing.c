#include <stdint.h>

typedef int uint16_t;

struct data
{
	unsigned int foo:4;
	unsigned int bar:4;
	uint8_t other;
	uint16_t other2;
};

int main(void)
{
	//return sizeof(struct data);
}

