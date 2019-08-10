// RUN: %ucc -o %t %s
// RUN: %t | %stdoutcheck %s
//      STDOUT: sts[0] = { 0, 0, 0 }
// STDOUT-NEXT: sts[1] = { 0, 0, 0 }
// STDOUT-NEXT: sts[2] = { 0, 0, 0 }
// STDOUT-NEXT: sts[3] = { 0, 0, 0 }
// STDOUT-NEXT: sts[4] = { 0, 0, 0 }
// STDOUT-NEXT: sts[5] = { 0, 1, 0 }

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	struct { int i, j, k; } sts[] = {
		[5] = { .j = 1 }
	};

	for(int i = 0; i < 6; i++)
		printf("sts[%d] = { %d, %d, %d }\n",
				i,
				sts[i].i, sts[i].j, sts[i].k);
}
