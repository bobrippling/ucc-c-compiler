// RUN: %output_check %s -fno-leading-underscore
// RUN: %ocheck 0 %s

void f();

void call_f()
{
	__asm("call %0" :: "i"(f)); // emit "call $f"
	// STDOUT: call_f:
	// STDOUT: call $f
	// STDOUT: ret
}

void prep_add()
{
	int x;

	/* --setup--
	 * foo=10
	 * bar=15
	 * load foo/10 -> eax
	 * load bar/15 -> ebx
	 */

	// STDOUT: prep_add:
	// STDOUT: mov.*10.*eax
	// STDOUT: mov.*15.*ebx
	// STDOUT: addl %ebx, %eax
	// STDOUT: mov eax.*(%rbp)
	// STDOUT: ret

	__asm__(
			"addl %%ebx, %%eax"
			: "=a"(x)
			: "a"(10)
			, "b"(15)
			);

	/* --tidy--
	 * store eax -> foo
	 */
}

void flag(int a, int b)
{
	// STDOUT: flag:
	// STDOUT: cmp
	// STDOUT: sete
	// STDOUT: got
	// STDOUT: ret

	__asm__("got %0 %1"
			: "+g"(*(int *)(a == b)) // mostly checking this doesn't cause a crash
			: "g"(a == b));
}

__attribute((always_inline))
inline void inlined(int i)
{
	__asm("inline value %0" : : "i"(i));
}

void do_inline()
{
	// STDOUT: do_inline:
	// STDOUT: inline value 3
	// STDOUT: ret
	f(3);
}

void memory_operand()
{
	int i;  // <--- 5
	int *p; // <--- &i

	// STDOUT: memory_operand:
	// STDOUT: movl $5, -8(%rbp)
	// STDOUT: ret

	// "=m"(i) - i written but also addressed
	__asm(
			"movl $5, %1\n\t"
			"lea %1, %0"
			: "=r"(p)
			, "=m"(i)
	);

	// "=m"(i) should mean that because 'i' is an lvalue we get its address directly
	if(p != &i){
		_Noreturn void abort(void);
		abort();
	}
}

void match_move()
{
	// STDOUT: match_move:
	// STDOUT: mov $3, %eax
	// STDOUT: mov %eax, -8(%rbp)
	// STDOUT: ret

	int i;
	__asm(""
			: "=r"(i)
			: "0"(3)
			);
}

int match(int *v)
{
	int ok;
	__asm(
			"mov %2, %0\r\n"
			"mov $3, %1"
			: "=&r" (ok) // TODO: what is this doing?
			, "=a" (*v)
			: "0" (10)); // testing this match
	return ok;
}

test_match()
{
	int x = 2;
	if(match(&x) != 10)
		abort();
	if(x != 3)
		abort();
	return 0;
}

void matching_constraint()
{
	// STDOUT: matching_constraint:
	// STDOUT: /a: 0=\(.*\) 1=\1/
	// STDOUT: /b: 0=\(.*\) 1=\1/
	// STDOUT: /c: 0=\(.*\) 1=\1/
	// STDOUT: ret

	int i = 2, j = 7;

	__asm("a: 0=%0 1=%1" : "=r"(i) : "0"(j));

	// j -> memory @ xyz
	// i <- memory @ xyz
	__asm("b: 0=%0 1=%1" : "=m"(i) : "0"(j));

	__asm("c: 0=%0 1=%1" : "=g"(i) : "0"(j));
}

void mem_indir()
{
	int i = 2;
	int *p = &i;

	__asm("movl $4, %0" : "=m"(*p));
	/* here it's possible to inline fully:
	 *
	 * movq <p>, %rax
	 * movl $4, (%rax)
	 */
	assert(i == 4);

	__asm("movl $5, %0" : "=r"(*p));
	/* but if the constraint is a register
	 * then we must do a post assignment from the register to *p
	 *
	 * movl $5, %ebx # rhs must be a register due to constraint
	 * movq <p>, %rax
	 * movl %ebx, (%rax)
	 */
	assert(i == 5);
}

int A, B;
void same()
{
	__asm("same_0 = %0\n"
			"same_1 = %1"
			: "=r"(A)
			: "r"(B)); // "A" and "B" may be the same register

	// STDOUT: same:
	// STDOUT: same_0 = %eax
	// STDOUT: same_1 = %eax
	// STDOUT: ret
}

void not_same_rw()
{
	__asm("not_same_rw_0 = %0\n"
			"not_same_rw_1 = %1"
			: "+r"(B)
			: "r"(A)); // "A" and "B" may not be the same register

	// STDOUT: not_same_rw:
	// STDOUT: not_same_rw_0 = %eax
	// STDOUT: not_same_rw_1 = %ecx
	// STDOUT: ret
}

void not_same_preclob()
{
	__asm("not_same_preclob_0 = %0\n"
			"not_same_preclob_1 = %1"
			: "=&r"(B)
			: "r"(A)); // "A" and "B" may not be the same register

	// STDOUT: not_same_preclob:
	// STDOUT: not_same_preclob_0 = %eax
	// STDOUT: not_same_preclob_1 = %eax
	// STDOUT: ret
}

void noop_toreg()
{
	// mov $3, %ecx
	__asm("" :: "c"(3));

	// STDOUT: noop_toreg:
	// STDOUT: movl $3, %ecx
	// STDOUT: ret
}

void operand_constraint_combinations()
{
	int i;

#define CHECK_ASM(cstraint_out, cstraint_in) \
	__asm("movl %1, %0" : cstraint_out(i) : cstraint_in(5)); \
	assert(i == 5)

	CHECK_ASM("=r", "r");
	CHECK_ASM("=g", "r");
	CHECK_ASM("=m", "r");

	CHECK_ASM("=r", "g");
	CHECK_ASM("=g", "g"); // works because we prefer reg to memory
	CHECK_ASM("=m", "g"); // ^ same

	CHECK_ASM("=r", "m");
	CHECK_ASM("=g", "m"); // ^ same
//CHECK_ASM("=m", "m");

	CHECK_ASM("=r", "i");
	CHECK_ASM("=g", "i");
	CHECK_ASM("=m", "i");
#undef CHECK_ASM
}

void ty_to_reg()
{
	long l;
	int i;
	short s;
	char c;

	__asm("; l=%0" : "=r"(l));
	__asm("; i=%0" : "=r"(i));
	__asm("; s=%0" : "=r"(s));
	__asm("; c=%0" : "=r"(c));

	// STDOUT: ty_to_reg:
	// STDOUT: l=%rax
	// STDOUT: i=%eax
	// STDOUT: s=%ax
	// STDOUT: c=%al
	// STDOUT: ret
}

void read_write()
{
	int i = 3;

	__asm("incl %0" : "+r"(i));
	assert(i == 4);

	__asm("addl %1, %0" : "+r"(i) : "g"(3));
	assert(i == 7);
}

void reg_xfer()
{
	int a = 2;

	__asm("" : "=r"(a) : "r"(5));
	assert(a == 5);

	__asm("" : "=r"(a) : "0"(1));
	assert(a == 1);
}

void regsel()
{
	int i = 3;

	// try not to choose a callee-save reg for "r"(3)
	__asm("; 0=%0\n"
			"; 1=%1"
			: "+r"(i)
			: "r"(3));

	// STDOUT: regsel:
	// STDOUT: 0=%eax
	// STDOUT: 1=%ecx
	// STDOUT: ret
}

void uniq()
{
	__asm("; u=%=" : );
	__asm("; u=%=" : );
	__asm("; u=%=" : );

	__asm("no interp %=");

	// STDOUT: uniq:
	// STDOUT: u=1
	// STDOUT: u=2
	// STDOUT: u=3
	// STDOUT: no interp %=
	// STDOUT: ret
}

__attribute((flatten))
void uniq_inline()
{
	uniq();
	// STDOUT: uniq_inline:
	// STDOUT: u=4
	// STDOUT: u=5
	// STDOUT: u=6
	// STDOUT: no interp %=
	// STDOUT: ret
}

int main()
{
	memory_operand();
	test_match();
	mem_indir();
	operand_constraint_combinations();
	read_write();
	reg_xfer();
}
