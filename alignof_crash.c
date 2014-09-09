//_Alignas(__alignof__(struct sockaddr *))
__attribute__((aligned((__alignof__(struct sockaddr *)))))

struct A
{
	unsigned short f;
	char data[128 - sizeof(unsigned short)];
};
