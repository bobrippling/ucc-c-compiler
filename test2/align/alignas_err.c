_Alignas(1) int i; // lowers alignment
_Alignas(3) char c; // not ^2
typedef _Alignas(8) int aligned_int;

_Alignas(2) f()
{
	_Alignas(8) register eax;
}
