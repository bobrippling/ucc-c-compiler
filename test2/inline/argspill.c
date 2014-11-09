// RUN: %ocheck 0 %s

extern __attribute((format(printf, 1, 2))) int printf(char *, ...);

niters;

__attribute((always_inline))
inline void iterate(int *begin, int *const end)
{
	int *i;
	for(i = begin; i != end; i++){
		printf("i=%p end=%p\n", i, end);
		niters++;
	}
}

int main()
{
	int ar[] = {
		1, 2
	};
	int *arend = ar + 2;

	printf("&end = %p\n", &arend);

	// arend would be in a register which wouldn't be saved
	// across basic blocks, causing issues
	iterate(ar, arend);

	if(niters != 2)
		abort();

	return 0;
}
