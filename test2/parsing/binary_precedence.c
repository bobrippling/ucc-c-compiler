f(int i)
{
	(void)(   0x3 ==((i & ~0xc) | 0x3)  );
	(void)(   0x3 == (i & ~0xc) | 0x3   );

	(void)(   5 >= 2 & 3   );
}
