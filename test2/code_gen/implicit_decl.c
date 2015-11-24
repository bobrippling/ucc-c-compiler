// RUN: %ucc -fdump-symtab -fsyntax-only %s 2>&1 | grep 'global,' | %output_check -w 'global, f' 'global, g' 'global, h'

void f()
{
	g();
}

void h()
{
	g();
}
