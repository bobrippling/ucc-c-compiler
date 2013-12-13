// RUN: %ucc -c %s; [ $? -ne 0 ]
// RUN: %ucc -fplan9-extensions -c %s

typedef union name
{
	int hex;
} Name;

struct plan9_extension
{
	Name;
} p9;

int *p_p9_ext  = &p9.hex;
int *p_p9_ext2 = &p9.Name.hex;

union name *pu = &p9.Name;

Name *pt = &p9.Name;
