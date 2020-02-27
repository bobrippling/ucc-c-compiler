// RUN: %ocheck 0 %s -lm
// RUN: %ucc -target x86_64-linux -S -o %t %s -DFOR_ASM
// RUN: %stdoutcheck %s <%t
// RUN: %stdoutcheck --prefix=constants %s <%t

#ifdef FOR_ASM
unsigned long long to_ull(double d)
{
	// STDOUT:      /^to_ull:/

	// STDOUT:      movsd float.1(%rip), %xmm1
	// STDOUT-NEXT: ucomisd %xmm1, %xmm0
	// STDOUT:      /ja \.Lblk\.[0-9]+/

	// STDOUT:      movsd float.2(%rip), %xmm0
	// STDOUT-NEXT: /movsd -[0-9]+\(%rbp\), %xmm1/
	// STDOUT-NEXT: subsd %xmm0, %xmm1
	// STDOUT-NEXT: cvttsd2siq %xmm1, %rax
	// STDOUT-NEXT: movabsq $9223372036854775808, %rcx
	// STDOUT-NEXT: xorq %rcx, %rax

	// STDOUT:     ret

	return d;
}

double from_ull(unsigned long long x)
{
	// STDOUT:      /^from_ull:/

	// STDOUT:      /movq %rax, -[0-9]+\(%rbp\)/
	// STDOUT-NEXT: js .Lblk.12

	// STDOUT:      cvtsi2sdq %rax, %xmm0
	// STDOUT-NEXT: /jmp \.Lblk\.[0-9]+/

	// STDOUT:      /movq -[0-9]+\(%rbp\), %rax/
	// STDOUT-NEXT: shrq $1, %rax
	// STDOUT-NEXT: /movq -[0-9]+\(%rbp\), %rcx/
	// STDOUT-NEXT: andq $1, %rcx
	// STDOUT-NEXT: orq %rcx, %rax
	// STDOUT-NEXT: cvtsi2sdq %rax, %xmm0
	// STDOUT-NEXT: addsd %xmm0, %xmm0

	// STDOUT:     ret

	return x;
}

long long to_ll(double d)
{
	// no test/conversion

	// STDOUT:     /^to_ll:/
	// STDOUT-NOT: /subsd|movabs|shr|or|add|ret/
	// STDOUT:     cvttsd2siq %xmm0, %rax
	// STDOUT:     ret

	return d;
}

double from_ll(long long x)
{
	// no test/conversion

	// STDOUT:     /^from_ll:/
	// STDOUT-NOT: /subsd|movabs|shr|or|add|ret/
	// STDOUT:     cvtsi2sdq %rax, %xmm0
	// STDOUT:     ret

	return x;
}

typedef unsigned T;
_Static_assert(sizeof(T) < 8, "need to test we don't convert for int32_t, etc");

T to_u(double d)
{
	// no test/conversion

	// STDOUT:     /^to_u:/
	// STDOUT-NOT: /subsd|movabs|shr|or|add|ret/
	// STDOUT:     cvttsd2sil %xmm0, %eax
	// STDOUT:     ret

	return d;
}

double from_u(T x)
{
	// no test/conversion

	// STDOUT:     /^from_u:/
	// STDOUT-NOT: /subsd|movabs|shr|or|add|ret/
	// STDOUT:     cvtsi2sdl %eax, %xmm0
	// STDOUT:     ret

	return x;
}

// STDOUT-constants:      float.1:
// STDOUT-NEXT-constants: /\.quad 4890909195324358656/
// STDOUT-constants:      float.2:
// STDOUT-NEXT-constants: /\.quad 4890909195324358656/

#else // ----------------------------------------

int eq_thru_unsigned(double d)
{
	return d == (unsigned long long)d;
}

int eq_thru_signed(double d)
{
	return d == (long long)d;
}

// ----------------------------------------

int printf(const char *, ...);

void assert(_Bool b, int l)
{
	void abort(void) __attribute((noreturn));
	if(!b){
		printf("fail %d\n", l);
		//abort();
	}
}
#define assert(c) assert((c), __LINE__)

void assert_to_double(unsigned long long l, double expected, int line)
{
	double d = l;
	//printf("got %a\n", d);
	(assert)(d == expected, line);
}
#define assert_to_double(...) assert_to_double(__VA_ARGS__, __LINE__)

void assert_to_ull(double d, unsigned long long expected, int line)
{
	unsigned long long x = d;
	//printf("got %#llx\n", x);
	(assert)(x == expected, line);
}
#define assert_to_ull(...) assert_to_ull(__VA_ARGS__, __LINE__)

double nextafter(double, double);

int main()
{
	// some of this invokes UB, but we define it as so:
	assert(!eq_thru_unsigned(-1));
	assert(eq_thru_signed(-1));

	// test all cases:
	// < 1p63, >= 1p63, negative
	assert_to_double(3, 3);
	assert_to_double(1ULL << 63, 0x1p63);
	assert_to_double(1ULL << 62, 0x1p62);
	assert_to_double(-3, 0x1p64);
	assert_to_double(-1ULL << 63, 0x1p63);
	assert_to_double(-1ULL << 62, 0x1.8p+63);

	assert_to_ull(3, 3);
	assert_to_ull(0x1p63, 1ULL << 63);
	assert_to_ull(0x2p63, 0);
	assert_to_ull(nextafter(0x1p63, 0), 0x7ffffffffffffc00);
	assert_to_ull(-3, -3);
	assert_to_ull(-0x1p63, -1ULL << 63);
	assert_to_ull(-0x2p63, 0x8000000000000000);
	assert_to_ull(nextafter(-0x1p63, 0), 0x8000000000000400);
}
#endif
