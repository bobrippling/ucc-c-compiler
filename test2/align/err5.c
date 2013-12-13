// RUN: ! %ucc -S -o- %s
_Alignas(2) f()
{
}
