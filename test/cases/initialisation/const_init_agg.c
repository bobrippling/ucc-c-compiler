// RUN: %check %s -std=c89 -w -Wc89-init-constexpr

g();

f()
{
	int i = g(); // CHECK: !/warn/
	int j = { g() }; // CHECK: !/warn/
	int k[] = { g() }; // CHECK: warning: aggregate initialiser is not a constant expression
	int l[] = {{ g() }}; // CHECK: warning: aggregate initialiser is not a constant expression
	struct { int x; } s = { g() }; // CHECK: warning: aggregate initialiser is not a constant expression
	struct { int x; } s2[] = { g() }; // CHECK: warning: aggregate initialiser is not a constant expression
	struct { int x; } s3[] = {{ g() }}; // CHECK: warning: aggregate initialiser is not a constant expression
}
