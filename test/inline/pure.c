// RUN: %ocheck 3 %s
// RUN: %ucc -S -o- %s | grep 'b:'

__attribute((noinline))
inline int b() // no code emitted yet
{
	return 3;
}
extern int b(); // causes b to be emitted externally

main()
{
	return b();
}
