int add (int a, int b) {
#ifndef __UCC__
    static
#endif
			const int offsets[] = {
        &&a0-&&a0, &&a1-&&a0, &&a2-&&a0
    };
    goto *(&&a0 + offsets[b]);
a0:
    return a + 0;
a1:
    return a + 1;
a2:
    return a + 2;
}

int main() {
	for(int i = 0; i <= 2; i++)
		printf("%d\n", add(5, i));
}
