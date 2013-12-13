// 9 * 9 =
// RUN: %ocheck 45 %s

typedef struct
{
	int x;
} int_1;

#define MAX_INTS 10

int main(void){
	int_1 *ints[MAX_INTS];

	for(int i = 0; i < MAX_INTS; i++)
		ints[i] = &(int_1){ i };
	// undefined once scope is left

	int sum = 0;
	for(int i = 0; i < MAX_INTS; i++)
		sum += ints[i]->x;


	return sum;
}
