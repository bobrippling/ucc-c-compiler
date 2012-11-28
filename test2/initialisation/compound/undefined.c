#include <stdio.h>

typedef struct int_struct
{
	int x;
} int_struct;

#define MAX_INTS 10

int main(void){
	size_t i;
	int_struct *ints[MAX_INTS];

	for (i = 0; i < MAX_INTS; i++) {
		ints[i] = &(int_struct){i}; // needs struct-init parsing/fold/gen
	}

	int sum = 0;
	for (i = 0; i < MAX_INTS; i++) {
		sum += ints[i]->x;
		//printf("%d\n", ints[i]->x);
	}

	return sum;
}
