// RUN: %check -e %s

// if at least one mentions static, the decl has internal linkage

// if a decl mentions extern:
	// prev decls visible: if they're internal or extern, the decl is as previous
	// no prev decls: external linkage

// no storage = extern

typedef void func(void);

// declared external to begin with - can't redeclare static

extern func internal;
func internal; // CHECK: note: previous definition
static func internal; // CHECK: error: static redefinition of non-static "internal"

void internal(void)
{
}



// declared nothing to begin with, then extern - can't redeclare static

func internal2;
extern func internal2; // CHECK: note: previous definition
static func internal2; // CHECK: error: static redefinition of non-static "internal2"

void internal2(void)
{
}



// declared extern to begin with - can't redeclare static

func internal3; // CHECK: note: previous definition
static func internal3; // CHECK: error: static redefinition of non-static "internal3"
extern func internal3;

void internal3(void)
{
}
