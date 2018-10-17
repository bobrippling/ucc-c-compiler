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

enum arch
{
	ARCH_x86_64,
	ARCH_i386,
};

enum vendor
{
	VENDOR_pc,
	VENDOR_apple,
};

enum sys
{
	SYS_linux,
	SYS_darwin,
	SYS_cygwin,
};

enum abi
{
	ABI_gnu,
	ABI_macho,
	ABI_elf,
};

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
