// RUN: %layout_check --sections --layout=%s.layout-zero-y %s -target x86_64-linux -fno-common -fno-zero-initialized-in-bss
// RUN: %layout_check --sections --layout=%s.layout-zero-n %s -target x86_64-linux -fno-common -fzero-initialized-in-bss

int tenative;
int explicit = 0;

struct {
	int i;
} tenative_s, explicit_s = { 0 };
