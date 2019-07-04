// RUN: %ucc -S -o %t %s
// RUN: test `grep ', same'             | sort | uniq | wc` -eq 1
// RUN: test `grep ', not_same_rw'      | sort | uniq | wc` -eq 1
// RUN: test `grep ', not_same_preclob' | sort | uniq | wc` -eq 1

int A, B;

same()
{
	__asm("mov %1, same\n\t"
			"mov %0, same"
			: "=r"(A)
			: "r"(B)); // "A" and "B" may be the same register
}

not_same_rw()
{
	__asm("mov %1, not_same_rw\n\t"
			"mov %0, not_same_rw"
			: "+r"(B)
			: "r"(A)); // "A" and "B" may not be the same register
}

not_same_preclob()
{
	__asm("mov %1, not_same_preclob\n\t"
			"mov %0, not_same_preclob"
			: "=&r"(B)
			: "r"(A)); // "A" and "B" may not be the same register
}
