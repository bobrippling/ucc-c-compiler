// RUN: %ucc -c %s
// RUN: test `%ucc -S -o- %s | grep -vF 'movq %%rsp, %%rbp' | grep -c 'mov'` -eq 1


extern int x[][2][4];

main()
{
	return x[1][2][3]; // only one dereference
}
