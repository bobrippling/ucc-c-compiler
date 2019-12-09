// RUN: %ocheck 0 %s
// RUN: %ucc -target x86_64-linux -S -o- %s -DFOR_ASM | %stdoutcheck %s

#ifdef FOR_ASM
unsigned long long to_ull(double d)
{
	// STDOUT:      /^to_ull:/

	// STDOUT:      movabsq $9223372036854775808, %rax
	// STDOUT-NEXT: cvtsi2sdq %rax, %xmm1
	// STDOUT-NEXT: ucomisd %xmm1, %xmm0
	// STDOUT:      /ja \.Lblk\.[0-9]+/

	// STDOUT:      movabsq $9223372036854775808, %rax
	// STDOUT-NEXT: cvtsi2sdq %rax, %xmm0
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

void assert(_Bool b)
{
	void abort(void) __attribute((noreturn));
	if(!b)
		abort();
}

int main()
{
	assert(!eq_thru_unsigned(-1));
	assert(eq_thru_signed(-1));
}
#endif
