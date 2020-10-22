// RUN: %ucc -fsyntax-only %s
f(int a)
{
	switch(a){
		case 1:
		case 2 ... 3:
		default:;
	}
}
