// RUN: %ucc -ffunction-sections -fdata-sections -S -o %t %s

struct A {
	int x;
}; // previously we'd crash here on trying to generate a section for this decl

int main() {
	return 3;
}
