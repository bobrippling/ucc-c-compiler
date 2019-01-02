// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 3 ]

main()
{
	void *jmps[3];

	int i = 0;

	jmps[0] = &&a, jmps[1] = &&b, jmps[2] = &&c;

rejmp:
	goto *jmps[i];

a:
	i = 1;
	goto rejmp;

b:
	i = 2;
	goto rejmp;

c:
	i = 3;
	goto fin;

fin:
	return i;
}
