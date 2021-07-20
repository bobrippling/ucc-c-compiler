// (disabled) RUN: true
// (disabled) RUN: %ocheck 0 %s

void assert(_Bool b)
{
	if(!b){
		void abort();
		abort();
	}
}

main()
{
#include "../ocheck-init.c"
	int arr[7] = {0,1,2,3,4,3,2};
	arr[0]++[arr]++[arr]++[arr]++[arr]++[arr]++[arr] = 5;

	assert(arr[0] == 2);
	assert(arr[1] == 3);
	assert(arr[2] == 4);
	assert(arr[3] == 5);
	assert(arr[4] == 4);
	assert(arr[5] == 3);
	assert(arr[6] == 2);
}
