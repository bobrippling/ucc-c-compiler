// RUN: %ocheck 0 %s

void assert(_Bool b, int lno, const char *exp)
{
	if(!b){
		_Noreturn void abort(void);
		extern int printf(const char *, ...);
		printf("%d %s\n", lno, exp);
	}
}
#define assert(b) assert(b, __LINE__, #b)

// --- structs ---

typedef struct A { int x; } A;

A struct_ = (A){ .x = 3 };

// --- unions ---

typedef union U { int x; long y; } U;

U union_ = (U){ .x = 4 };

// --- arrays ---

int ar_inferred[] = (int[]){ 1, 2, 3 };
int ar_eqsize[1] = (int[1]){ 5 };
int ar_empty[0] = (int[0]){};
int ar_empty2[] = (int[0]){};
int ar_empty3[] = (int[]){};

int main()
{
	assert(struct_.x == 3);
	assert(union_.x == 4);

	_Static_assert(sizeof(ar_inferred) == sizeof(int) * 3, "");
	assert(ar_inferred[0] == 1);
	assert(ar_inferred[1] == 2);
	assert(ar_inferred[2] == 3);

	_Static_assert(sizeof(ar_eqsize) == sizeof(int) * 1, "");
	assert(ar_eqsize[0] == 5);

	_Static_assert(sizeof(ar_empty) == 0, "");
	_Static_assert(sizeof(ar_empty2) == 0, "");
	_Static_assert(sizeof(ar_empty3) == 0, "");
}
