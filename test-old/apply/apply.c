/*
 * apply.c
 *
 * A basic implementation of apply() for systems that use the
 * System V AMD64 calling convention.
 */

#define _BSD_SOURCE
#include <string.h>
#include <sys/mman.h>

size_t codesize(size_t argc);
char* setup_stack(char* buf, size_t argc);
char* inject_param(char* buf, size_t pos, void* arg);
char* inject_funcall(char* buf, void* func);
void restore_stack(char* buf, size_t argc);

void* apply(void* func, void** args, size_t argc) {
	size_t len = codesize(argc);
	char* buf;
	void* result;
	char* codemem = mmap(0, len,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANON/*YMOUS*/, -1, 0);
	if (!codemem) {
		return NULL;
	}

	buf = setup_stack(codemem, argc);
	for (size_t pos=0; pos < argc; ++pos) {
		buf = inject_param(buf, pos, args[pos]);
	}
	buf = inject_funcall(buf, func);
	restore_stack(buf, argc);

	result = ((void* (*)()) codemem)();
	munmap(codemem, len);
	return result;
}

size_t codesize(size_t argc) {
	if (argc <= 6) {
		return (argc * 10) + 7 + 1;
	} else {
		return 8 + 60 + ((argc - 6) * 16) + 7 + 2;
	}
}

char* setup_stack(char* buf, size_t argc) {
	if (argc <= 6) {
		return buf;
	} else {
		/* push %rbp */
		buf[0] = 0x55;

		/* mov %rsp, %rbp */
		buf[1] = 0x48;
		buf[2] = 0x89;
		buf[3] = 0xe5;

		/* sub ..., %rsp */
		buf[4] = 0x48;
		buf[5] = 0x83;
		buf[6] = 0xec;

		/* Supports up to 31 stack arguments. */
		buf[7] = (unsigned char) ((argc - 6) * 8);
		return buf + 8;
	}
}

char* inject_param(char* buf, size_t pos, void* arg) {
	/* RDI, RSI, RDX, RCX, R8, R9, <Stack> */
	if (pos < 6) {
		const unsigned char movs[] = {
			0xbf, /* rdi */
			0xbe, /* rsi */
			0xba, /* rdx */
			0xb9, /* rcx */
			0xb8, /* r8 */
			0xb9, /* r9 */
		};
		buf[0] = (pos < 4) ? 0x48 : 0x49;
		buf[1] = movs[pos];
		memcpy(buf + 2, &arg, 8);
		return buf + 10;
	} else {
		/* movl ...[0:4], ...(%rsp) */
		/* movl ...[4:8], ...+4(%rsp) */
		char* param;
		buf[0] = buf[8] = 0xc7;
		buf[1] = buf[9] = 0x44;
		buf[2] = buf[10] = 0x24;
		buf[3] = (unsigned char) ((pos - 6) * 8);
		buf[11] = buf[3] + 4;
		param = (void*) &arg;
		memcpy(buf + 4, param, 4);
		memcpy(buf + 12, param + 4, 4);
		return buf + 16;
	}
}

char* inject_funcall(char* buf, void* func) {
	/* mov ..., %eax */
	buf[0] = 0xb8;
	memcpy(buf + 1, &func, 4);

	/* callq *%rax */
	buf[5] = 0xff;
	buf[6] = 0xd0;
	return buf + 7;
}

void restore_stack(char* buf, size_t argc) {
	if (argc > 6) {
		/* leaveq */
		buf[0] = 0xc9;
		buf = buf + 1;
	}

	/* retq */
	buf[0] = 0xc3;
}
