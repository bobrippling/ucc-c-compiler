#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/utsname.h>

#include "target.h"
#include "alloc.h"

#define X(pre, post)         \
	if(!strcmp(in, #post)){    \
		*out = pre ## _ ## post; \
		return 1;                \
	}

#define X_ncmp(pre, post, n)  \
	if(!strncmp(in, #post, n)){ \
		*out = pre ## _ ## post;  \
		return 1;                 \
	}

static int parse_arch(const char *in, enum arch *out)
{
	TARGET_ARCHES
	return 0;
}

static int parse_vendor(const char *in, enum vendor *out)
{
	TARGET_VENDORS
	return 0;
}

static int parse_sys(const char *in, enum sys *out)
{
	TARGET_SYSES
	return 0;
}

static int parse_abi(const char *in, enum abi *out)
{
	TARGET_ABIS
	return 0;
}
#undef X
#undef X_ncmp

static enum vendor infer_vendor(enum sys sys)
{
	switch(sys){
		case SYS_linux: return VENDOR_pc;
		case SYS_darwin: return VENDOR_apple;
		case SYS_cygwin: return VENDOR_pc;
	}
	return -1;
}

static enum abi infer_abi(enum sys sys)
{
	switch(sys){
		case SYS_linux: return ABI_elf;
		case SYS_darwin: return ABI_macho;
		case SYS_cygwin: return ABI_elf;
	}
	return -1;
}

int triple_parse(const char *str, struct triple *triple, const char **const bad)
{
	char *dup = ustrdup(str);
	char *tok;
	enum arch arch = -1;
	enum vendor vendor = -1;
	enum sys sys = -1;
	enum abi abi = -1;

	for(tok = strtok(dup, "-"); tok; tok = strtok(NULL, "-")){
		if(parse_arch(tok, &arch))
			continue;
		if(parse_vendor(tok, &vendor))
			continue;
		if(parse_sys(tok, &sys))
			continue;
		if(parse_abi(tok, &abi))
			continue;

		*bad = ustrdup(tok);
		return 0;
	}

	if(arch == (enum arch)-1){
		*bad = "no arch";
		return 0;
	}
	if(sys == (enum sys)-1){
		*bad = "no sys";
		return 0;
	}

	if(vendor == (enum vendor)-1)
		vendor = infer_vendor(sys);
	if(abi == (enum abi)-1)
		abi = infer_abi(sys);

	triple->arch = arch;
	triple->vendor = vendor;
	triple->sys = sys;
	triple->abi = abi;
	return 1;
}

static const char *arch_to_str(enum arch a)
{
	switch(a){
		case ARCH_x86_64: return "x86_64";
		case ARCH_i386: return "i386";
	}
}

static const char *vendor_to_str(enum vendor v)
{
	switch(v){
		case VENDOR_pc: return "pc";
		case VENDOR_apple: return "apple";
	}
}

static const char *sys_to_str(enum sys s)
{
	switch(s){
		case SYS_linux: return "linux";
		case SYS_darwin: return "darwin";
		case SYS_cygwin: return "cygwin";
	}
}

static const char *abi_to_str(enum abi a)
{
	switch(a){
		case ABI_gnu: return "gnu";
		case ABI_macho: return "macho";
		case ABI_elf: return "elf";
	}
}

char *triple_to_str(const struct triple *triple)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "%s-%s-%s-%s",
			arch_to_str(triple->arch),
			vendor_to_str(triple->vendor),
			sys_to_str(triple->sys),
			abi_to_str(triple->abi));

	return buf;
}

static void filter_unam(struct utsname *unam)
{
	char *p;

	if(!strcmp(unam->machine, "amd64"))
		strcpy(unam->machine, "x86_64");

	for(p = unam->sysname; *p; p++)
		*p = tolower(*p);
}

int triple_default(struct triple *triple)
{
	struct utsname unam;

	if(uname(&unam))
		return 0;

	filter_unam(&unam);

	if(!parse_arch(unam.machine, &triple->arch))
		return 0;
	if(!parse_sys(unam.sysname, &triple->sys))
		return 0;

	triple->vendor = infer_vendor(triple->sys);
	triple->abi = infer_abi(triple->sys);

	return 1;
}
