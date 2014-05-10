// RUN: %ucc -fsyntax-only %s

// check the ctx arguments don't clash:
int a(void *ctx, void cb(void *ctx));
int b(void cb(void *ctx), void *ctx);

f(int x)
{
	{
		int x = 3;
	}
}
