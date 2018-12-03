#ifndef UTIL_TARGET_H
#define UTIL_TARGET_H

/*
The triple has the general format <arch><sub>-<vendor>-<sys>-<abi>, where:

        arch = x86_64, i386, arm, thumb, mips, etc.
        sub = for ex. on ARM: v5, v6m, v7a, v7m, etc.
        vendor = pc, apple, nvidia, ibm, etc.
        sys = none, linux, win32, darwin, cuda, etc.
        abi = eabi, gnu, android, macho, elf, etc.

<arch><sub>-<vendor>-<sys>-<abi>
    |          |       |     |
    |          |       |     +---> --print-supported-abis
    |          |       +---------> --print-supported-systems
    |          +-----------------> --print-supported-vendors
    +----------------------------> --print-supported-archs

 [^^^^^^^^^^^^^^+^^^^^^^^^^^^^^^]
                |
                +---> --print-available-targets

Examples:
	x86_64-linux-gnu
	x86_64-apple-darwin17.6.0
	arm-linux-gnueabihf

*/

#define TARGET_ARCHES \
	X(ARCH, x86_64) \
	X(ARCH, i386)   \
	ALIAS(ARCH, amd64, x86_64)

#define TARGET_VENDORS \
	X(VENDOR, pc) \
	X(VENDOR, apple)

#define TARGET_SYSES \
	X(SYS, linux) \
	X(SYS, freebsd) \
	X_ncmp(SYS, darwin, 6) \
	X_ncmp(SYS, cygwin, 6)

#define TARGET_ABIS \
	X(ABI, gnu) \
	X(ABI, macho) \
	X(ABI, elf)

#define X(pre, post) pre ## _ ## post,
#define X_ncmp(pre, post, n) X(pre, post)
#define ALIAS(...)
enum arch
{
	TARGET_ARCHES
};

enum vendor
{
	TARGET_VENDORS
};

enum sys
{
	TARGET_SYSES
};

enum abi
{
	TARGET_ABIS
};
#undef X
#undef X_ncmp
#undef ALIAS

struct triple
{
	enum arch arch;
	enum vendor vendor;
	enum sys sys;
	enum abi abi;
};

int triple_parse(const char *str, struct triple *triple, const char **const bad);

int triple_default(struct triple *);
char *triple_to_str(const struct triple *);

#endif
