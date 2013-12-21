// RUN: %ucc -DSTORE= %s; [ $? -ne 0 ]
// RUN: %ucc -DSTORE=static %s
// RUN: %ucc -DSTORE=extern %s

int a;

main()
{
	STORE int a;
	static int *p = &a;
}
