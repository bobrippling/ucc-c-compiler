#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/utsname.h>

#include "triple.h"
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

#define ALIAS(pre, from, to) \
	if(!strcmp(in, #from)){    \
		*out = pre ## _ ## to;   \
		return 1;                \
	}

static int parse_mainarch(const char *in, enum arch *out)
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
#undef ALIAS

static int parse_subarch(
		const char *in,
		const enum arch chosen_arch,
		enum subarch *out)
{
	const size_t in_len = strlen(in);

#define X(pre, arch, sub) \
	if(chosen_arch == ARCH_ ## arch \
			&& in_len > strlen(#arch) \
			&& !strncmp(in + strlen(#arch), #sub, strlen(#sub))) \
	{ \
		*out = pre ## _ ## arch ## sub; \
		return 1; \
	}
#define NONE(pre)
	TARGET_SUBARCHES
#undef X
#undef NONE
	return 0;
}

static int parse_arch(const char *in, enum arch *arch, enum subarch *subarch)
{
	if(!parse_mainarch(in, arch))
		return 0;

	if(!parse_subarch(in, *arch, subarch))
		*subarch = SUBARCH_none;
	return 1;
}

static enum vendor infer_vendor(enum sys sys)
{
	switch(sys){
		case SYS_linux: return VENDOR_pc;
		case SYS_darwin: return VENDOR_apple;
		case SYS_cygwin: return VENDOR_pc;
		case SYS_freebsd: return VENDOR_pc;
	}
	return -1;
}

static enum abi infer_abi(enum sys sys)
{
	switch(sys){
		case SYS_linux: return ABI_gnu;
		case SYS_darwin: return ABI_macho;
		case SYS_cygwin: return ABI_elf;
		case SYS_freebsd: return ABI_elf;
	}
	return -1;
}

int triple_parse(const char *str, struct triple *triple, const char **const bad)
{
	char *dup = ustrdup(str);
	char *tok;
	enum arch arch = -1;
	enum subarch subarch = -1;
	enum vendor vendor = -1;
	enum sys sys = -1;
	enum abi abi = -1;

	for(tok = strtok(dup, "-"); tok; tok = strtok(NULL, "-")){
		if(parse_arch(tok, &arch, &subarch))
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
	triple->subarch = subarch;
	triple->vendor = vendor;
	triple->sys = sys;
	triple->abi = abi;
	return 1;
}

static const char *arch_to_str(enum arch a)
{
	switch(a){
#define X(pre, name) case pre ## _ ## name: return #name;
#define X_ncmp(pre, post, n) X(pre, post)
#define ALIAS(...)
		TARGET_ARCHES
#undef ALIAS
#undef X_ncmp
#undef X
	}
	return NULL;
}

static const char *subarch_to_str(enum subarch s)
{
	switch(s){
#define X(pre, arch, sub) case pre ## _ ## arch ## sub: return #sub;
#define NONE(pre)  case pre ## _ ## none: return "";
		TARGET_SUBARCHES
#undef X
#undef NONE
	}
	return NULL;
}

static const char *vendor_to_str(enum vendor v)
{
	switch(v){
#define X(pre, name) case pre ## _ ## name: return #name;
		TARGET_VENDORS
#undef X
	}
	return NULL;
}

const char *triple_sys_to_str(enum sys s)
{
	switch(s){
#define X(pre, name) case pre ## _ ## name: return #name;
#define X_ncmp(pre, name, n) X(pre, name)
		TARGET_SYSES
#undef X_ncmp
#undef X
	}
	return NULL;
}

const char *triple_abi_to_str(enum abi a)
{
	switch(a){
#define X(pre, name) case pre ## _ ## name: return #name;
		TARGET_ABIS
#undef X
	}
	return NULL;
}

char *triple_to_str(const struct triple *triple, int showvendor)
{
	static char buf[32];

	snprintf(buf, sizeof(buf), "%s%s%s%s-%s-%s",
			arch_to_str(triple->arch),
			subarch_to_str(triple->subarch),
			showvendor ? "-" : "",
			showvendor ? vendor_to_str(triple->vendor) : "",
			triple_sys_to_str(triple->sys),
			triple_abi_to_str(triple->abi));

	return buf;
}

static void filter_unam(struct utsname *unam)
{
	char *p;

#define X(pre, name)
#define ALIAS(pre, from, to) \
	if(!strcmp(unam->machine, #from)) \
		strcpy(unam->machine, #to);
#define X_ncmp(pre, post, n)
	TARGET_ARCHES
#undef X_ncmp
#undef ALIAS
#undef X

	for(p = unam->sysname; *p; p++)
		*p = tolower(*p);
}

int triple_default(struct triple *triple, const char **const unparsed)
{
	struct utsname unam;

	if(unparsed)
		*unparsed = NULL;

	if(uname(&unam))
		return 0;

	filter_unam(&unam);

	if(parse_arch(unam.machine, &triple->arch, &triple->subarch)){
		if(triple->arch == ARCH_arm && triple->subarch == SUBARCH_armv7){
			/* since we're defaulting, pick armv6 for compatability */
			triple->subarch = SUBARCH_armv6;
		}
	}else{
		if(unparsed)
			*unparsed = unam.machine;
		return 0;
	}
	if(!parse_sys(unam.sysname, &triple->sys)){
		if(unparsed)
			*unparsed = unam.sysname;
		return 0;
	}

	triple->vendor = infer_vendor(triple->sys);
	triple->abi = infer_abi(triple->sys);

	return 1;
}
