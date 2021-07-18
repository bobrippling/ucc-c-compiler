// RUN: %ucc -Wno-undefined-internal -fpic -S -o %t %s
// RUN: grep -F 'vis_default@GOTPCREL(%%rip)' %t
// RUN: grep -F 'vis_hidden(%%rip)' %t
// RUN: grep -F 'vis_static(%%rip)' %t
// RUN: grep -F 'vis_extern@GOTPCREL(%%rip)' %t

typedef void vfn(void);

__attribute((visibility("default"))) vfn vis_default;
//__attribute((visibility("protected"))) vfn vis_protected;
__attribute((visibility("hidden"))) vfn vis_hidden;
static vfn vis_static;
extern vfn vis_extern;

void f(vfn *);

int main()
{
	f(&vis_default);
	//f(&vis_protected);
	f(&vis_hidden);
	f(&vis_static);
	f(&vis_extern);
}
