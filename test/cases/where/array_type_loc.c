// RUN: %check %s

inteq(int a, int b);
int *find(int *begin, int *end, int n, int cmp(int, int));
void bad();

int main()
{
	int ar[] = { // CHECK: note: array declared here
		1, 2, 3, 4, 5
	};

	int *arend = ar + 5;
	int *end = find(ar, arend, 0, inteq);

	if(end != &ar[5]) // CHECK: warning: index 5 out of bounds of array, size 5
		bad();
}
