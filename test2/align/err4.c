// RUN: ! %ucc -S -o- %s
f()
{
	_Alignas(8) register eax;
}
