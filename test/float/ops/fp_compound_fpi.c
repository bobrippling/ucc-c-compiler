// RUN: %ucc -S -o- %s | grep addss > /dev/null
// RUN: %ucc -S -o- %s | grep addl > /dev/null; [ $? -ne 0 ]

f()
{
	float a = 0;

	a += 1; // 1 cast to float, then added
}
