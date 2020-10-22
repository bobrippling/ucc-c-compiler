// RUN: %ucc -S -o- %s | grep addl > /dev/null; [ $? -ne 0 ]
// RUN: %ucc -S -o- %s | grep addss > /dev/null

f(float f)
{
	int i = 0;

	i += f; // i loaded, cast to float, added, cast to int and stored
}
