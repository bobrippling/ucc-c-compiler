// RUN: %layout_check --sections --layout=%s.layout-pic    %s -fno-common -target x86_64-linux -fpic
// RUN: %layout_check --sections --layout=%s.layout-no-pic %s -fno-common -target x86_64-linux -fno-pic

const int ki = 3; // ro
int i = 3;        // data

const int *const p1 = &ki;       // relro
      int *const p2 = &(int){3}; // relro
const int *const p3 = 0;         // ro

int *q = &i; // data

const char *const str = "hi"; // relro

// -----------------------

typedef struct { int x[3]; } Array;

Array array; // bss

// no operator&, check that struct-exprs are handled
int *const wrap_p1 = array.x; // relro

int (*const wrap_p2)[] = &array.x; // relro

int *const wrap_p3 = &array.x[1]; // relro

// -----------------------

Array arrays[5]; // bss

typedef struct { Array *ar1; Array (*ar2)[]; } Ref;

const Ref r = { // relro
	.ar1 = arrays + 3,
	.ar2 = &arrays + 1,
};

// -----------------------

typedef struct { int x, y; } Int;

int *const p = &((Int *)0)->y; // ro

// -----------------------

void *const pv1 = _Generic(
		0,
		int: arrays[3].x + 2,
		char: 3);

void *const pv2 = _Generic(
		0,
		char: &(*r.ar2)[3].x[1], /* not a constant expr, never mind relocatable */
		int: (int[]){ 3 });

// -----------------------

void (^bpmut)(void) = ^{};
void (^const bp)(void) = ^{};
