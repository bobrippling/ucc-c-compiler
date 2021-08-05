void f(unsigned);
void g(int);

int main()
{
#ifdef MOV
	// mov
	f(0x11000000);
	f(0x00110000);
	f(0x00001100);
	f(0x00000011);
	f(0x10000001);

	// movw+movt / ldr
	f(0x11010000);
	f(0x01110000);
	f(0x01001100);
	f(0x10000011);
	f(0x11000001);
#endif

#ifdef MVN
	// mvn
	g(-1);
	g(-2);
	g(-5);
	g(-305419896); //  -0x12345678
#endif
}
